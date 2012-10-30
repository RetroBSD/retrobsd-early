/*
 * File main.c: 2.7 (84/11/28,10:14:56)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "data.h"

main(int argc, char *argv[]) {
    char *param = NULL, *bp;
    int smacptr, i;
    macptr = 0;
    ctext = 0;
    errs = 0;
    aflag = 1;
    uflag = 0;
    for (i=1; i<argc; i++) {
        param = argv[i];
        if (*param == '-') {
            while (*++param) {
                switch (*param) {
                    case 't': case 'T': // output c source as asm comments
                        ctext = 1;
                        break;
                    case 's': case 'S': // assemble .s files - do #define ASNM to make it work
                        sflag = 1;
                        break;
                    case 'c': case 'C': // linker - this option does not work
                        cflag = 1;
                        break;
                    case 'a': case 'A': // no argument count in A to function calls
                        aflag = 0;
                        break;
                    case 'u': case 'U': // use undocumented 8085 instructions
                        uflag = 1;
                        break;
                    case 'd': case 'D': // define macro
                        bp = ++param;
                        if (!*param) usage();
                        while (*param && *param != '=') param++;
                        if (*param == '=') *param = '\t';
                        while (*param) param++;
                        param--;
                        defmac(bp);
                        break;
                    default:
                        usage();
                }
            }
        } else {
            break;
        }
    }

    smacptr = macptr; // command line defined macros -d
    //if (!param) {
    //    usage();
    //}
    if (i == argc) {
        compile(NULL); // training mode - read code from stdin
        exit(0);
    }

    for (; i<argc; i++) {
        param = argv[i];
        errfile = 0;
        macptr = smacptr;
        compile(param);
    }
#ifndef NOASLD
    if (!errs && !sflag && !cflag)
        errs = errs || link();
#endif
    exit(errs != 0);
}

/**
 * compile one file if filename is NULL redirect do to stdin/stdout
 * @param file filename
 * @return 
 */
compile(char *file) {
    if (file == NULL || filename_typeof(file) == 'c') {
        global_table_index = 1; //glbptr = STARTGLB;
        local_table_index = NUMBER_OF_GLOBALS; //locptr = STARTLOC;
        while_table_index = 0; //wsptr = ws;
        inclsp =
        iflevel =
        skiplevel =
        swstp =
        litptr =
        stkp =
        errcnt =
        ncmp =
        lastst =
        //quote[1] =
        0;
        input2 = NULL;
        //quote[0] = '"';
        cmode = 1;
        glbflag = 1;
        nxtlab = 0;
        litlab = getlabel();
        defmac("end\tmemory");
        //add_global("memory", ARRAY, CCHAR, 0, EXTERN);
        //add_global("stack", ARRAY, CCHAR, 0, EXTERN);
        rglobal_table_index = global_table_index; //rglbptr = glbptr;
        //add_global("etext", ARRAY, CCHAR, 0, EXTERN);
        //add_global("edata", ARRAY, CCHAR, 0, EXTERN);
        defmac("short\tint");
//        initmac();
        create_initials();
        // compiler body
        if (file == NULL) {
            input = stdin;
        } else if (!openin(file))
            return;
        if (file == NULL) {
            output = stdout;
        } else if (!openout())
            return;
        header();
        code_segment_gtext();
        parse();
        fclose(input);
        data_segment_gdata();
        dumplits();
        dumpglbs();
        errorsummary();
        trailer();
        fclose(output);
        pl("");
        errs = errs || errfile;
#ifndef NOASLD
    }
    if (!errfile && !sflag) {
        errs = errs || assemble(file);
    }
#else
    } else {
        fputs("Don't understand file ", stderr);
        fputs(file, stderr);
        errs = 1;
    }
#endif
}

frontend_version() {
    output_string("\tFront End (2.7,84/11/28)");
}

/**
 * prints usage
 * @return exits the execution
 */
usage() {
    fputs("usage: sccXXXX [-tcsah] [-dSYM[=VALUE]] files\n", stderr);
    fputs("-t: output c source as asm comments\n", stderr);
    fputs("-a: no argument count in A to function calls\n", stderr);
    fputs("-d: define macro\n", stderr);
    fputs("-u: use undocumented 8085 instructions\n", stderr);
    fputs("-s: assemble generated output, not implemented\n", stderr);
    fputs("-c: link, not implemented\n", stderr);
    fputs("-h: displays usage\n", stderr);
    fputs("files - one or more files. no filename redirects to stdin/stdout\n", stderr);
    exit(1);
}

/**
 * process all input text
 * at this level, only static declarations, defines, includes,
 * and function definitions are legal.
 */
parse() {
    while (!feof(input)) {
        if (amatch("extern", 6))
            dodcls(EXTERN);
        else if (amatch("static", 6))
            dodcls(STATIC);
        else if (dodcls(PUBLIC));
        else if (match("#asm"))
            doasm();
        else if (match("#include"))
            doinclude();
        else if (match("#define")) {
            dodefine();
        }
        else if (match("#undef"))
            doundef();
        else {
            newfunc();
        }
        blanks();
    }
}

/**
 * parse top level declarations
 * @param stclass
 * @return 
 */
dodcls(int stclass) {
    int type;
    blanks();
    if (type = get_type()) {
        declare_global(type, stclass);
    } else if (stclass == PUBLIC) {
        return (0);
    } else {
        declare_global(CINT, stclass);
    }
    need_semicolon();
    return (1);
}

/**
 * dump the literal pool
 */
dumplits() {
    int j, k;

    if (litptr == 0)
        return;
    print_label(litlab);
    output_label_terminator();
    k = 0;
    while (k < litptr) {
        gen_def_byte();
        j = 8;
        while (j--) {
            output_number(litq[k++] & 127);
            if ((j == 0) | (k >= litptr)) {
                newline();
                break;
            }
            output_byte(',');
        }
    }
}

/**
 * dump all static variables
 */
dumpglbs() {
    int dim, i, list_size, line_count, value;

    if (!glbflag)
        return;
    current_symbol_table_idx = rglobal_table_index;
    while (current_symbol_table_idx < global_table_index) {
        SYMBOL *symbol = &symbol_table[current_symbol_table_idx];
        if (symbol->identity != FUNCTION) {
            ppubext(symbol);
            if (symbol->storage != EXTERN) {
				prefix();
                output_string(symbol->name);
                output_label_terminator();
                dim = symbol->offset;
                list_size = 0;
                line_count = 0;
                if (find_symbol(symbol->name)) { // has initials
                    list_size = get_size(symbol->name);
                    if (dim == -1) {
                        dim = list_size;
                    }
                }
                for (i=0; i<dim; i++) {
                    if (line_count % 10 == 0) {
                        newline();
                        if ((symbol->type & CINT) || (symbol->identity == POINTER)) {
                            gen_def_word();
                        } else {
                            gen_def_byte();
                        }
                    }
                    if (i < list_size) {
                        // dump data
                        value = get_item_at(symbol->name, i);
                        output_number(value);
                    } else {
                        // dump zero, no more data available
                        output_number(0);
                    }
                    line_count++;
                    if (line_count % 10 == 0) {
                        line_count = 0;
                    } else {
                        if (i < dim-1) {
                            output_byte( ',' );
                        }
                    }
                }
                newline();
            }
        } else {
            fpubext(symbol);
        }
        current_symbol_table_idx++;
    }
}

/*
 *      report errors
 */
errorsummary() {
    if (ncmp)
        error("missing closing bracket");
    newline();
    gen_comment();
    output_decimal(errcnt);
    if (errcnt) errfile = YES;
    output_string(" error(s) in compilation");
    newline();
    gen_comment();
    output_with_tab("literal pool:");
    output_decimal(litptr);
    newline();
    gen_comment();
    output_with_tab("global pool:");
    output_decimal(global_table_index - rglobal_table_index);
    newline();
    gen_comment();
    output_with_tab("Macro pool:");
    output_decimal(macptr);
    newline();
    if (errcnt > 0)
        pl("Error(s)");
}

/**
 * test for C or similar filename, e.g. xxxxx.x, tests the dot at end-1 postion
 * @param s the filename
 * @return the last char if it contains dot, space otherwise
 */
filename_typeof(char *s) {
    s += strlen(s) - 2;
    if (*s == '.')
        return (*(s + 1));
    return (' ');
}

