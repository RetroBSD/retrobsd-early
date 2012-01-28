(* Copyright (c)2011 Ashley Feniello *)

open System
open System.IO
open System.Text
 
let MEM_SIZE = 1024 * 1024
let IMAGE_FILE = "retroImage"
let MAX_OPEN_FILES = 8
let PORTS = 12
 
let mutable ip = 0
let mutable ports = Array.create PORTS 0
let mutable inputs = Array.create PORTS ""
let mutable memory = Array.create MEM_SIZE 0
let mutable (files : FileStream[]) = Array.create MAX_OPEN_FILES null
let mutable isp = 0
let mutable offset = 0
let mutable shrink = false
let mutable halt = false
 
let data = ref []
let address = ref []
 
let pushVal d x = d := x :: !d
let popVal d () = match !d with h :: t -> d := t; h | _ -> failwith "Underflow"
let push = pushVal data
let pushr = pushVal address
let pop = popVal data
let popr = popVal address
let tos () = (!data).Head
 
let load() =
    use binReader = new BinaryReader(File.Open(IMAGE_FILE, FileMode.Open))
    for i in 0 .. int (binReader.BaseStream.Length / 4L) - 1 do
        memory.[i] <- binReader.ReadInt32()
 
let saveImage () =
    let j = if shrink then memory.[3] else MEM_SIZE
    use binWriter = new BinaryWriter(File.Open(IMAGE_FILE, FileMode.Create))
    Array.iter (fun (c : int) -> binWriter.Write(c)) memory.[0..j - 1]
 
let key () =
    if isp > 0 && offset = inputs.[isp].Length - 1 then // Next input source?
        isp <- isp - 1
        offset <- 0
    if isp > 0 then // Read from a file
        offset <- offset + 1
        int inputs.[isp].[offset]
    else // Read from Console
        let cki = Console.ReadKey(true)
        if cki.Key = ConsoleKey.Backspace then printf "\b "
        int cki.KeyChar
 
let devices () =
    let getString () =
        let s = pop ()
        let e = Array.FindIndex(memory, s, fun c -> c = 0)
        new String(Array.map char memory.[s .. e - 1])
    let openFile () =
        let handle = Array.findIndex ((=) null) files |> ((+) 1)
        let mode, name = pop (), getString ()
        try
            match mode with
            | 0 -> files.[handle] <- File.Open(name, FileMode.Open)
            | 1 -> files.[handle] <- File.Open(name, FileMode.OpenOrCreate)
            | 2 -> files.[handle] <- File.Open(name, FileMode.Append)
            | 3 -> let f = File.Open(name, FileMode.Open)
                   f.Seek(0L, SeekOrigin.End) |> ignore
                   files.[handle] <- f
            | _ -> failwith "Invalid mode"
            handle
        with _ -> 0
    let readFile h =
        let c = files.[h].ReadByte()
        if c = -1 then 0 else c
    let writeFile h = pop () |> byte |> files.[h].WriteByte; 1
    let closeFile h =
        let f = files.[h]
        if f <> null then
            f.Close()
            f.Dispose()
            files.[h] <- null
        0
    let getFilePos h = int files.[h].Position
    let setFilePos h = files.[h].Seek(pop () |> int64, SeekOrigin.Begin) |> int
    let getFileSize handle = files.[handle].Length |> int
    let deleteFile name = if File.Exists name then File.Delete name; -1 else 0
    if ports.[0] <> 1 then
        ports.[0] <- 1
        if ports.[1] = 1 then ports.[1] <- key () // Read from input source
        if ports.[2] = 1 then
            let x = pop ()
            if x < 0 then Console.Clear()
            else Console.Write(char x)
            ports.[2] <- 0
        if ports.[3] = 1 then ports.[3] <- 0 // Video update
        match ports.[4] with
        | 1 -> saveImage () ; ports.[4] <- 0 // Save Image
        | 2 -> // Add to Input Stack
            isp <- isp + 1
            inputs.[isp] <- System.IO.File.ReadAllText(getString ())
            ports.[4] <- 0
        | -1 -> ports.[4] <- openFile ()
        | -2 -> ports.[4] <- pop () |> readFile
        | -3 -> ports.[4] <- pop () |> writeFile
        | -4 -> ports.[4] <- pop () |> closeFile
        | -5 -> ports.[4] <- pop () |> getFilePos
        | -6 -> ports.[4] <- pop () |> setFilePos
        | -7 -> ports.[4] <- pop () |> getFileSize
        | -8 -> ports.[4] <- getString () |> deleteFile
        | _ -> ports.[4] <- 0
        ports.[5] <- match ports.[5] with // Capabilities
            | -1 -> MEM_SIZE
            | -5 -> (!data).Length // stack depth
            | -6 -> (!address).Length // address stack depth
            | -8 -> int (DateTime.UtcNow - new DateTime(1970,1,1)).TotalSeconds
            | -9 -> halt <- true; 0
            | -10 -> // Query for environment variable
                let var = getString ()
                let name = ref (pop ())
                Array.iter (fun element ->
                    memory.[name.Value] <- int element
                    name := name.Value + 1)
                    (var |> Environment.GetEnvironmentVariable
                         |> Encoding.ASCII.GetBytes)
                0
            | -11 -> Console.WindowWidth
            | -12 -> Console.WindowHeight
            | _ -> 0
 
let rec exec () =
    let dyadic fn = let x = pop () in fn (pop ()) x |> push
    let dyadic2 fn = let x, y = fn (pop ()) (pop ()) in push y; push x
    let incIp () = ip <- ip + 1
    let condJump fn =
        let x = pop ()
        if fn (pop ()) x then ip <- memory.[ip + 1] - 1 else incIp ()
    let jump () =
        ip <- memory.[ip] - 1
        if memory.[ip + 1] = 0 then incIp ()
        if memory.[ip + 1] = 0 then incIp ()
    let drop () = pop () |> ignore
    let loop () =
        pop () - 1 |> push
        if tos () > 0 then ip <- memory.[ip + 1] - 1
        else incIp (); drop ()
    if not halt then
        match memory.[ip] with
            | 0 -> () // NOP
            | 1 -> incIp (); memory.[ip] |> push // LIT
            | 2 -> tos () |> push // DUP
            | 3 -> drop () // DROP
            | 4 -> dyadic2 (fun x y -> y, x) // SWAP
            | 5 -> pop () |> pushr // PUSH
            | 6 -> popr () |> push // POP
            | 7 -> loop () // LOOP
            | 8 -> incIp (); jump () // JUMP
            | 9 -> ip <- popr () // RETURN
            | 10 -> condJump (>) // GT_JUMP
            | 11 -> condJump (<) // LT_JUMP
            | 12 -> condJump (<>) // NE_JUMP
            | 13 -> condJump (=) // EQ_JUMP
            | 14 -> memory.[pop ()] |> push // FETCH
            | 15 -> memory.[pop ()] <- pop () // STORE
            | 16 -> dyadic (+) // ADD
            | 17 -> dyadic (-) // SUB
            | 18 -> dyadic (*) // MUL
            | 19 -> dyadic2 (fun x y -> y / x, y % x) // DIVMOD
            | 20 -> dyadic (&&&) // AND
            | 21 -> dyadic (|||) // OR
            | 22 -> dyadic (^^^) // XOR
            | 23 -> dyadic (<<<) // SHL
            | 24 -> dyadic (>>>) // SHR
            | 25 -> if tos () = 0 then drop (); ip <- popr () // ZERO_EXIT
            | 26 -> pop () + 1 |> push // INC
            | 27 -> pop () - 1 |> push // DEC
            | 28 -> let x = pop () in ports.[x] |> push; ports.[x] <- 0  // IN
            | 29 -> ports.[pop ()] <- pop () // OUT
            | 30 -> devices () // WAIT
            | _ -> pushr ip; jump ()
        ip <- ip + 1
        exec ()
 
let args = Environment.GetCommandLineArgs()
Array.iteri
    (fun i arg ->
        match arg with
        | "--shrink" -> shrink <- true
        | "--about" -> printfn "Retro Language [VM: F#, .NET]"
        | "--with" ->
            isp <- isp + 1
            inputs.[isp] <- File.ReadAllText(args.[i + 1])
        | _ -> ()
    ) args
 
load ()
exec ()
