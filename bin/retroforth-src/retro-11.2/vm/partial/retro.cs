/*****************************************************************************
 * Ngaro for Mono / .NET
 *
 * Copyright (c) 2009 - 2011, Simon Waite and Charles Childers
 *
 * Please compile with `gmcs` as `mcs` seems to have a
 * simple Console.in implementation.
 *****************************************************************************/

using System;
using System.IO;
using System.Text;

namespace Retro.Ngaro
{
  public class VM
  {
    /* Registers */
    int sp, rsp, ip, shrink;
    int[] data, address, ports, memory;

    /* Input Stack  */
    string[] inputs;
    int[] lengths;
    int isp, offset;


    /* Opcodes recognized by the VM */
    enum OpCodes {
      VM_NOP,      VM_LIT,        VM_DUP,
      VM_DROP,     VM_SWAP,       VM_PUSH,
      VM_POP,      VM_LOOP,       VM_JUMP,
      VM_RETURN,   VM_GT_JUMP,    VM_LT_JUMP,
      VM_NE_JUMP,  VM_EQ_JUMP,    VM_FETCH,
      VM_STORE,    VM_ADD,        VM_SUB,
      VM_MUL,      VM_DIVMOD,     VM_AND,
      VM_OR,       VM_XOR,        VM_SHL,
      VM_SHR,      VM_ZERO_EXIT,  VM_INC,
      VM_DEC,      VM_IN,         VM_OUT,
      VM_WAIT
    }


    /* Initialize the VM */
    public VM() {
      sp = 0;
      rsp = 0;
      ip = 0;
      data    = new int[128];
      address = new int[1024];
      ports   = new int[12];
      memory  = new int[1000000];

      inputs  = new string[12];
      lengths = new int[12];
      isp = 0;
      offset = 0;

      loadImage();

      if (memory[0] == 0) {
        Console.Write("Sorry, unable to find retroImage\n");
        Environment.Exit(0);
      }
    }


    /* Load the 'retroImage' into memory */
    public void loadImage() {
      int i = 0;
      if (!File.Exists("retroImage"))
        return;

      BinaryReader binReader = new BinaryReader(File.Open("retroImage", FileMode.Open));
      FileInfo f = new FileInfo("retroImage");
      long s = f.Length / 4;
      try {
        while (i < s) { memory[i] = binReader.ReadInt32(); i++; }
      }
      catch(EndOfStreamException e) {
        Console.WriteLine("{0} caught and ignored." , e.GetType().Name);
      }
      finally {
        binReader.Close();
      }
    }


    /* Save the image */
    public void saveImage() {
      int i = 0, j = 1000000;
      BinaryWriter binWriter = new BinaryWriter(File.Open("retroImage", FileMode.Create));
      try {
        if (shrink != 0)
          j = memory[3];
        while (i < j) { binWriter.Write(memory[i]); i++; }
      }
      catch(EndOfStreamException e) {
        Console.WriteLine("{0} caught and ignored." , e.GetType().Name);
      }
      finally {
        binWriter.Close();
      }
    }


    /* Read a key */
    public int read_key() {
      int a = 0;

      /* Check to see if we need to move to the next input source */
      if (isp > 0 && offset == lengths[isp]) {
        isp--;
        offset = 0;
      }

      if (isp > 0) {      /* Read from a file */
        a = (int)inputs[isp][offset];
        offset++;
      }
      else {              /* Read from Console */
        ConsoleKeyInfo cki = Console.ReadKey();
        a = (int)cki.KeyChar;
        if (cki.Key == ConsoleKey.Backspace) {
          a = 8;
          Console.Write((char)32);
        }
        if ( a >= 32)
          Console.Write((char)8);
      }
      return a;
    }


    /* Handle I/O device emulation */
    public void HandleDevices() {
      if (ports[0] == 1)
        return;

      ports[0] = 1;

      /* Read from input source */
      if (ports[1] == 1) {
        int a = read_key();
        ports[1] = a;
      }

      /* Write to display */
      if (ports[2] == 1) {
        char c = (char)data[sp];
        if (data[sp] < 0)
          Console.Clear();
        else
          Console.Write(c);
        sp--;
        ports[2] = 0;
      }

      /* File Operations: Save Image */
      if (ports[4] == 1) {
        saveImage();
        ports[4] = 0;
      }

      /* File Operations: Add to Input Stack */
      if (ports[4] == 2) {
        ports[4] = 0;
        int i = 0;
        char[] s = new char[1024];
        int name = data[sp]; sp--;
        while(memory[name] != 0) {
          s[i] = (char)memory[name];
          i++; name++;
        }
        isp++;
        inputs[isp]  = System.IO.File.ReadAllText(new String(s,0,i));
        lengths[isp] = inputs[isp].Length;
      }

      /* Capabilities */
      if (ports[5] == -1)
        ports[5] = 1000000;
      if (ports[5] == -2)
        ports[5] = 0;
      if (ports[5] == -3)
        ports[5] = 0;
      if (ports[5] == -4)
        ports[5] = 0;
      if (ports[5] == -5)
        ports[5] = sp;
      if (ports[5] == -6)
        ports[5] = rsp;
      if (ports[5] == -7)
        ports[5] = 0;
      if (ports[5] == -8) {
        int unixTime = (int)(DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalSeconds;
        ports[5] = unixTime;
      }
      if (ports[5] == -9) {
        ports[5] = 0;
        ip = 1000000;
      }
      if (ports[5] == -10) {
        char[] s = new char[1024];
        int name = data[sp]; sp--;
        int i = 0;
        while(memory[name] != 0) {
          s[i] = (char)memory[name];
          i++; name++;
        }
        byte[] array = Encoding.ASCII.GetBytes(Environment.GetEnvironmentVariable(new String(s,0,i)));
        name = data[sp]; sp--;
        foreach (byte element in array) {
          memory[name] = (int)element;
          name++;
        }
        ports[5] = 0;
      }
      if (ports[5] == -11)
        ports[5] = Console.WindowWidth;
      if (ports[5] == -12)
        ports[5] = Console.WindowHeight;
    }


    /* Process the current opcode */
    public void Process() {
      int x, y;

      switch((OpCodes)memory[ip])
      {
        case OpCodes.VM_NOP:
          break;
        case OpCodes.VM_LIT:
          sp++; ip++; data[sp] = memory[ip];
          break;
        case OpCodes.VM_DUP:
          sp++; data[sp] = data[sp-1];
          break;
        case OpCodes.VM_DROP:
          data[sp] = 0; sp--;
          break;
        case OpCodes.VM_SWAP:
          x = data[sp];
          y = data[sp-1];
          data[sp] = y;
          data[sp-1] = x;
          break;
        case OpCodes.VM_PUSH:
          rsp++;
          address[rsp] = data[sp];
          sp--;
          break;
        case OpCodes.VM_POP:
          sp++;
          data[sp] = address[rsp];
          rsp--;
          break;
        case OpCodes.VM_LOOP:
          data[sp]--;
          if (data[sp] != 0 && data[sp] > -1)
            ip = memory[ip + 1] - 1;
          else {
            ip++;
            sp--;
          }
          break;
        case OpCodes.VM_JUMP:
          ip++;
          ip = memory[ip] - 1;
          if (memory[ip+1] == 0)
            ip++;
          if (memory[ip+1] == 0)
            ip++;
          break;
        case OpCodes.VM_RETURN:
          ip = address[rsp]; rsp--;
          break;
        case OpCodes.VM_GT_JUMP:
          ip++;
          if (data[sp-1] > data[sp])
            ip = memory[ip] - 1;
          sp = sp - 2;
          break;
        case OpCodes.VM_LT_JUMP:
          ip++;
          if (data[sp-1] < data[sp])
            ip = memory[ip] - 1;
          sp = sp - 2;
          break;
        case OpCodes.VM_NE_JUMP:
          ip++;
          if (data[sp-1] != data[sp])
            ip = memory[ip] - 1;
          sp = sp - 2;
          break;
        case OpCodes.VM_EQ_JUMP:
          ip++;
          if (data[sp-1] == data[sp])
            ip = memory[ip] - 1;
          sp = sp - 2;
          break;
        case OpCodes.VM_FETCH:
          x = data[sp];
          data[sp] = memory[x];
          break;
        case OpCodes.VM_STORE:
          memory[data[sp]] = data[sp-1];
          sp = sp - 2;
          break;
        case OpCodes.VM_ADD:
          data[sp-1] += data[sp]; data[sp] = 0; sp--;
          break;
        case OpCodes.VM_SUB:
          data[sp-1] -= data[sp]; data[sp] = 0; sp--;
          break;
        case OpCodes.VM_MUL:
          data[sp-1] *= data[sp]; data[sp] = 0; sp--;
          break;
        case OpCodes.VM_DIVMOD:
          x = data[sp];
          y = data[sp-1];
          data[sp] = y / x;
          data[sp-1] = y % x;
          break;
        case OpCodes.VM_AND:
          x = data[sp];
          y = data[sp-1];
          sp--;
          data[sp] = x & y;
          break;
        case OpCodes.VM_OR:
          x = data[sp];
          y = data[sp-1];
          sp--;
          data[sp] = x | y;
          break;
        case OpCodes.VM_XOR:
          x = data[sp];
          y = data[sp-1];
          sp--;
          data[sp] = x ^ y;
          break;
        case OpCodes.VM_SHL:
          x = data[sp];
          y = data[sp-1];
          sp--;
          data[sp] = y << x;
          break;
        case OpCodes.VM_SHR:
          x = data[sp];
          y = data[sp-1];
          sp--;
          data[sp] = y >>= x;
          break;
        case OpCodes.VM_ZERO_EXIT:
          if (data[sp] == 0) {
            sp--;
            ip = address[rsp]; rsp--;
          }
          break;
        case OpCodes.VM_INC:
          data[sp]++;
          break;
        case OpCodes.VM_DEC:
          data[sp]--;
          break;
        case OpCodes.VM_IN:
          x = data[sp];
          data[sp] = ports[x];
          ports[x] = 0;
          break;
        case OpCodes.VM_OUT:
          ports[data[sp]] = data[sp-1];
          sp = sp - 2;
          break;
        case OpCodes.VM_WAIT:
          HandleDevices();
          break;
        default:
          rsp++;
          address[rsp] = ip;
          ip = memory[ip] - 1;
          if (memory[ip+1] == 0)
            ip++;
          if (memory[ip+1] == 0)
            ip++;
          break;
      }
    }


    /* Process the image until the IP reaches the end of memory */
    public void Execute() {
      for (; ip < 1000000; ip++)
         Process();
    }


    /* Main entry point */
    /* Calls all the other stuff and process the command line */
    public static void Main(string [] args) {
      VM vm = new VM();
      vm.shrink = 0;

      for (int i = 0; i < args.Length; i++) {
        if (args[i] == "--shrink")
          vm.shrink = 1;
        if (args[i] == "--about") {
          Console.Write("Retro Language  [VM: C#, .NET]\n\n");
          Environment.Exit(0);
        }
        if (args[i] == "--with") {
          i++;
          vm.isp++;
          vm.inputs[vm.isp] = System.IO.File.ReadAllText(args[i]);
          vm.lengths[vm.isp] = vm.inputs[vm.isp].Length;
        }
      }
      vm.Execute();
    }
  }
}
