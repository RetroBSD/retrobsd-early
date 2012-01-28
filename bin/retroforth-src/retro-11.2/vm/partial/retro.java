/*
 * Copyright (c) 2009 - 2011, Simon Waite and Charles Childers
 * Based on the C# implementation
*/

import java.*;
import java.io.*;

public class retro {
  int sp, rsp, ip;
  int data[], address[], ports[];
  int memory[];

  public static final int VM_NOP = 0;       public static final int VM_LIT = 1;       public static final int VM_DUP = 2;
  public static final int VM_DROP = 3;      public static final int VM_SWAP = 4;      public static final int VM_PUSH = 5;
  public static final int VM_POP = 6;       public static final int VM_LOOP = 7;      public static final int VM_JUMP = 8;
  public static final int VM_RETURN = 9;    public static final int VM_GT_JUMP = 10;  public static final int VM_LT_JUMP = 11;
  public static final int VM_NE_JUMP = 12;  public static final int VM_EQ_JUMP = 13;  public static final int VM_FETCH = 14;
  public static final int VM_STORE = 15;    public static final int VM_ADD = 16;      public static final int VM_SUB = 17;
  public static final int VM_MUL = 18;      public static final int VM_DIVMOD = 19;   public static final int VM_AND = 20;
  public static final int VM_OR = 21;       public static final int VM_XOR = 22;      public static final int VM_SHL = 23;
  public static final int VM_SHR = 24;      public static final int VM_ZERO_EXIT = 25;public static final int VM_INC = 26;
  public static final int VM_DEC = 27;      public static final int VM_IN = 28;       public static final int VM_OUT = 29;
  public static final int VM_WAIT = 30;


 /**
  * Returns the value in the opposite endian
  *
  * @return int
  */
  public int switchEndian(int value)
  {
    int b1 = (value >>  0) & 0xff;
    int b2 = (value >>  8) & 0xff;
    int b3 = (value >> 16) & 0xff;
    int b4 = (value >> 24) & 0xff;
    return b1 << 24 | b2 << 16 | b3 << 8 | b4 << 0;
  }


 /**
  * Load an image file. Will do nothing if the file does
  * not exist.
  */
  public void loadImage(String name)
  {
    int i;
    RandomAccessFile in = null;

    boolean exists = (new File(name)).exists();
    if (exists)
    {
      try
      {
        in = new RandomAccessFile(name, "r");
        i = 0;

        while (i < (in.length()/4))
        {
          memory[i] = switchEndian(in.readInt()); i++;
        }
        if (in != null)
          in.close();
      }
      catch(Exception e) { System.out.println(e); }
    }
    else
    {
      memory[0] = 0;
    }
  }


 /**
  * Save the current image to a file.
  */
  public void saveImage(String name)
  {
    int i;

    try
    {
      RandomAccessFile out = new RandomAccessFile(name, "rw");
      i = 0;

      while (i < 1000000)
      {
        out.writeInt(switchEndian(memory[i])); i++;
      }
      if (out != null)
        out.close();
    }
    catch(Exception e) { System.out.println(e); }
  }


 /**
  * Generic constructor
  */
  public void retro()
  {
  }



 /**
  * Attempt to load the initial image. This tries to load a
  * file (retroImage). If that fails, it uses a built-in one.
  */
  public void initial_image()
  {
    int i;
    for(i = 0; i < 1000000; i++)
      memory[i] = 0;

    loadImage("retroImage");
    if (memory[0] == 0)
    {
      System.out.println("Could not find image file!");
      System.exit(-1);
    }
  }

/**
 * Initialize the Ngaro VM. Should this be merged into
 * the constructor?
 */
  public void init()
  {
    sp = 0;
    rsp = 0;
    ip = 0;
    data    = new int[128];
    address = new int[1024];
    ports   = new int[12];
    memory  = new int[1000000];
    initial_image();
  }


 /**
  * Handler for the emulated hardware devices
  */
  public void handleDevices()
  {
    byte[] b = { 0, 0, 0 };
    char c = ' ';
    int d = 0;

    if (ports[0] == 1)
      return;

    if (ports[0] == 0 && ports[1] == 1)
    {
      try { System.in.read(b, 0, 1); } catch(Exception e) { System.out.println(e); }
      ports[1] = (int)b[0];
      ports[0] = 1;
    }
    if (ports[2] == 1)
    {
      c = (char)data[sp];
      if (data[sp] < 0)
      {
        for(c = 0; c < 300; c++)
          System.out.println("\n");
      }
      else
        System.out.print(c);
      sp--;
      ports[2] = 0;
      ports[0] = 1;
    }
    if (ports[4] == 1)
    {
      saveImage("retroImage");
      ports[4] = 0;
      ports[0] = 1;
    }

    /* Capabilities */
    if (ports[5] == -1)
    {
      ports[5] = 1000000;
      ports[0] = 1;
    }
    if (ports[5] == -2)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -3)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -4)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -5)
    {
      ports[5] = sp;
      ports[0] = 1;
    }
    if (ports[5] == -6)
    {
      ports[5] = rsp;
      ports[0] = 1;
    }
    if (ports[5] == -7)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -8)
    {
      ports[5] = (int) (System.currentTimeMillis() / 1000L);
      ports[0] = 1;
    }
    if (ports[5] == -9)
    {
      ip = 1000000;
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -10)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -11)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -12)
    {
      ports[5] = 0;
      ports[0] = 1;
    }
    if (ports[5] == -13)
    {
      ports[5] = 32;
      ports[0] = 1;
    }
    if (ports[5] == -14)
    {
      ports[5] = 1;
      ports[0] = 1;
    }
  }


 /**
  * Process a single opcode
  */
  public void process()
  {
    int op = memory[ip];
    int x, y, z;

    switch(memory[ip])
    {
    case VM_NOP:
      break;
    case VM_LIT:
      sp++; ip++; data[sp] = memory[ip];
      break;
    case VM_DUP:
      sp++; data[sp] = data[sp-1];
      break;
    case VM_DROP:
      data[sp] = 0; sp--;
      break;
    case VM_SWAP:
      x = data[sp];
      y = data[sp-1];
      data[sp] = y;
      data[sp-1] = x;
      break;
    case VM_PUSH:
      rsp++;
      address[rsp] = data[sp];
      sp--;
      break;
    case VM_POP:
      sp++;
      data[sp] = address[rsp];
      rsp--;
      break;
    case VM_LOOP:
      data[sp]--;
      ip++;
      if (data[sp] != 0 && data[sp] > -1)
        ip = memory[ip] - 1;
      else
        sp--;
      break;
    case VM_JUMP:
      ip++;
      ip = memory[ip] - 1;
      if (memory[ip + 1] == 0)
        ip++;
      if (memory[ip + 1] == 0)
        ip++;
      break;
    case VM_RETURN:
      ip = address[rsp]; rsp--;
      if (memory[ip + 1] == 0)
        ip++;
      if (memory[ip + 1] == 0)
        ip++;
      break;
    case VM_GT_JUMP:
      ip++;
      if (data[sp-1] > data[sp])
        ip = memory[ip] - 1;
      sp = sp - 2;
      break;
    case VM_LT_JUMP:
      ip++;
      if (data[sp-1] < data[sp])
        ip = memory[ip] - 1;
      sp = sp - 2;
      break;
    case VM_NE_JUMP:
      ip++;
      if (data[sp-1] != data[sp])
        ip = memory[ip] - 1;
      sp = sp - 2;
      break;
    case VM_EQ_JUMP:
      ip++;
      if (data[sp-1] == data[sp])
        ip = memory[ip] - 1;
      sp = sp - 2;
      break;
    case VM_FETCH:
      x = data[sp];
      data[sp] = memory[x];
      break;
    case VM_STORE:
      memory[data[sp]] = data[sp-1];
      sp = sp - 2;
      break;
    case VM_ADD:
      data[sp-1] += data[sp]; data[sp] = 0; sp--;
      break;
    case VM_SUB:
      data[sp-1] -= data[sp]; data[sp] = 0; sp--;
      break;
    case VM_MUL:
      data[sp-1] *= data[sp]; data[sp] = 0; sp--;
      break;
    case VM_DIVMOD:
      x = data[sp];
      y = data[sp-1];
      data[sp] = y / x;
      data[sp-1] = y % x;
      break;
    case VM_AND:
      x = data[sp];
      y = data[sp-1];
      sp--;
      data[sp] = x & y;
      break;
    case VM_OR:
      x = data[sp];
      y = data[sp-1];
      sp--;
      data[sp] = x | y;
      break;
    case VM_XOR:
      x = data[sp];
      y = data[sp-1];
      sp--;
      data[sp] = x ^ y;
      break;
    case VM_SHL:
      x = data[sp];
      y = data[sp-1];
      sp--;
      data[sp] = y << x;
      break;
    case VM_SHR:
      x = data[sp];
      y = data[sp-1];
      sp--;
      data[sp] = y >>= x;
      break;
    case VM_ZERO_EXIT:
      if (data[sp] == 0)
      {
        sp--;
        ip = address[rsp]; rsp--;
      }
      break;
    case VM_INC:
      data[sp]++;
      break;
    case VM_DEC:
      data[sp]--;
      break;
    case VM_IN:
      x = data[sp];
      data[sp] = ports[x];
      ports[x] = 0;
      break;
    case VM_OUT:
      ports[0] = 0;
      ports[data[sp]] = data[sp-1];
      sp = sp - 2;
      break;
    case VM_WAIT:
      handleDevices();
      break;
    default:
      rsp++;
      address[rsp] = ip;
      ip = memory[ip] - 1;
      if (memory[ip + 1] == 0)
        ip++;
      if (memory[ip + 1] == 0)
        ip++;
      break;
    }
  }


 /**
  * The main entry point. What else needs to be said?
  */
  public static void main(String[] args)
  {
    retro vm = new retro();
    vm.init();
    for (vm.ip = 0; vm.ip < 1000000; vm.ip++)
       vm.process();
  }
}
