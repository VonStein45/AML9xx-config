/* 
 * ARMBIAN BOOT CONFIGURATION TOOL (Intended for AML-S9XX-box, but could probably be used for other ARMbian images)
 * ----------------------------------------------------------------------------------------------------------------
 * 
 * Standard C-code that will compile using any C compiler (ZIG (all platforms), MINGW (windows), standard linux gcc (linux))
 *
 * Since space on the vfat boot partition on an SD card is limited, it should preferably be 
 * compiled with size optimization and using static linking (linux: Use musl library).
 *  
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

const char *U_DEST = "./u-boot.ext";
const char *E_CONF = "./extlinux/extlinux.conf";
const char *E_BAK  = "./extlinux/extlinux.conf.bak";
const char *E_TMP  = "./extlinux/extlinux.new";
const char *D_DIR  = "./dtb/amlogic";

int f_exists(const char *fn) {
    struct stat b;
    return (stat(fn, &b) == 0);
}

void f_copy(const char *src, const char *dst) {
    FILE *s;
    FILE *d;
    unsigned char buffer[4096];
    size_t n;
    
    if ( (s = fopen(src, "rb")) == NULL) return;
    if ( (d = fopen(dst, "wb")) == NULL) return;
    
    while ((n = fread(buffer, 1, sizeof(buffer), s)) > 0) fwrite(buffer, 1, n, d);

    fclose(s); fclose(d);
}

/**
 * ur_choice: Prompts user for a numeric choice within [min, max].
 * Manual stdin parsing avoids linking scanf to keep binary size down.
 */
int ur_choice(int min, int max, const char *fmt, ...) {
    enum {UR_REMOVE_LEADING_WS, UR_HANDLE_DIGITS, UR_REMOVE_TRAILING_WS} ps_state;
    
    while (1) {
        // 1. Print formatted prompt
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);
            fflush(stdout);
        }

        int val = 0;
        int count = 0;
        int ch;
        int done = 0;

        // 2. Manual parsing
        ps_state = UR_REMOVE_LEADING_WS;
        while (!done) {
            ch = getchar();
/*            printf("(%02x)\n", ch);*/
            switch (ps_state) {
                case UR_REMOVE_LEADING_WS:
                    if (isspace(ch)) break;  /* jump over leading whitespace(s) */
                    ps_state = UR_HANDLE_DIGITS;
                    /* Note: falls through switch */
                case UR_HANDLE_DIGITS:
                    if (!isspace(ch)) {
                        if (!isdigit(ch)) { count == 99; break;}   /* Force error */
                    
                        if (ch >= '0' && ch <= '9' && count < 4) {  /* count<4= handles up to 1000 */
                            val = val * 10 + (ch - '0');
                        }
                        count++;
                        break;
                    }
                    ps_state = UR_REMOVE_TRAILING_WS;
                    /* Note: falls through switch */
                case UR_REMOVE_TRAILING_WS:
                    if (ch == '\n') { done = 1; break; }
                    if (!isspace(ch)) { count = 99; break;}  /* Force error */
                    break;
            }
        }
        
        // 3. Validation: If valid, return immediately
        if (count > 0 && count <= 4 && val >= min && val <= max) {
            return val;
        }

        // 4. Otherwise, print error and the loop naturally restarts the prompt
        if (fmt != NULL) printf("Error: Input must be a number between %d and %d.\n", min, max);
    }
}

#define MAX_FILES 1024
#define MAX_FNLEN (sizeof(((struct dirent *)0)->d_name)/sizeof(char))

/* file name structure for dir-listings */
typedef struct {
    char fname[MAX_FILES][MAX_FNLEN];
    int n;
} DIR_LIST;

DIR_LIST global_dir_list;  /* Must NOT be static, since that increases executable size with the size of the allocated memory */

void bubblesort_global_dir_list() {
    char temp;
    int dirty = 1;
    
    while (dirty) {
        dirty = 0;
        for (int i = 0; i < global_dir_list.n - 1; i++) {
            /* Compare filenames */
            if (strcmp(global_dir_list.fname[i], global_dir_list.fname[i+1]) > 0) {
                dirty = 1;
                
                /* three-way swap byte for byte */
                for (int k = 0; k < MAX_FNLEN; k++) {
                    temp = global_dir_list.fname[i][k];
                    global_dir_list.fname[i][k] = global_dir_list.fname[i+1][k];
                    global_dir_list.fname[i+1][k] = temp;
                }
            }
        }
    }
}


void do_uboot() {
    struct dirent *e;
    int i, choice;
    DIR *d;

    d = opendir(".");
    if (!d) return;

    puts("\n--- SELECT U-BOOT BINARY ---\n");
    puts("Scanning root directory for u-boot files...\n");

    global_dir_list.n = 0;
    while ((e = readdir(d)) != NULL && global_dir_list.n < MAX_FILES) {
         if ((strncmp(e->d_name, "u-boot", 6) == 0) && (strncmp(e->d_name, "u-boot.", 7) != 0)) {
            strncpy(global_dir_list.fname[global_dir_list.n], e->d_name, MAX_FNLEN);
            printf("%d) %s\n", global_dir_list.n+1, global_dir_list.fname[global_dir_list.n]);
            global_dir_list.n++;
        }
    }
    closedir(d);

    bubblesort_global_dir_list();

    if (global_dir_list.n > 0) {
        choice = ur_choice(0, global_dir_list.n, "Choice (0-%d)...(0=Cancel and return): ", global_dir_list.n);
        if (choice > 0) {
            printf("Applying %s as %s...\n", global_dir_list.fname[choice-1], U_DEST);
            f_copy(global_dir_list.fname[choice-1], U_DEST);
            printf("Success!\n");
        }
    } else {
        puts("No compatible u-boot files found.\n");
    }
}

void do_dtb() {
    struct dirent *e;
    int i, choice;
    unsigned char ln[1024];
    enum {LS_PRE_WS, LS_DTB_TST_F, LS_DTB_TST_D, LS_DTB_TST_T, LS_PST_WS, LS_DTB_FN, LS_COPY_REST} lnstate;

    if (!f_exists(E_BAK)) {
        if (f_exists(E_CONF)) {
            puts("First run: Backing up original extlinux.conf...\n");
            f_copy(E_CONF, E_BAK);
        } else {
            printf("Error: %s not found!\n", E_CONF);
            return;
        }
    }

    DIR *d = opendir(D_DIR);
    if (!d) { printf("Error: DTB directory %s missing\n", D_DIR); return; }

    printf("\n--- SELECT DEVICE TREE BLOB (DTB) ---\n");
    global_dir_list.n = 0;
    while ((e = readdir(d)) != NULL && global_dir_list.n < MAX_FILES) {
        if (strstr(e->d_name, ".dtb")) {
            strncpy(global_dir_list.fname[global_dir_list.n], e->d_name, MAX_FNLEN);
            printf("%3d) %s\n", global_dir_list.n+1, global_dir_list.fname[global_dir_list.n]);
            global_dir_list.n++;
        }
    }
    closedir(d);

    bubblesort_global_dir_list();

    if (global_dir_list.n > 0) {
        choice = ur_choice(0, global_dir_list.n, "Choice (0-%d)...(0=Cancel and return): ", global_dir_list.n);
        if (choice > 0) {
            FILE *in = fopen(E_CONF, "r");
            FILE *ot = fopen(E_TMP, "w");
            if(in && ot) {
                while (fgets(ln, sizeof(ln), in)) {
                    lnstate = LS_PRE_WS;
                    for (i=0; ln[i] != '\0'; i++) {
                        switch (lnstate) {
                            case LS_PRE_WS:
                                if (isspace(ln[i])) {
                                    fputc(ln[i], ot);  /* Copy leading whitespace(s) */
                                    break;
                                }
                                if (ln[i] == '#') {
                                    fputc(ln[i], ot);  /* comment - copy rest of the line */
                                    lnstate = LS_COPY_REST;
                                    break;
                                }
                                lnstate = LS_DTB_TST_F;
                                /* NOTE: Parser falls through to LS_DTB_TST_F */
                            case LS_DTB_TST_F:
                                lnstate = (toupper(ln[i]) == 'F') ? LS_DTB_TST_D: LS_COPY_REST;  /* != 'F'=Abort pattern recognition and copy rest of line */
                                fputc(ln[i], ot);                                
                                break;
                            case LS_DTB_TST_D:
                                lnstate = (toupper(ln[i]) == 'D') ? LS_DTB_TST_T: LS_COPY_REST;  /* != 'D'=Abort pattern recognition and copy rest of line */
                                fputc(ln[i], ot);                                  
                                break;
                            case LS_DTB_TST_T:
                                lnstate = (toupper(ln[i]) == 'T') ? LS_PST_WS: LS_COPY_REST;     /* != 'T'=Abort pattern recognition and copy rest of line */
                                fputc(ln[i], ot);                                  
                                break;
                            case LS_PST_WS:
                                if (isspace(ln[i])) {
                                    fputc(ln[i], ot);  /* Copy intermediate whitespace(s) */
                                    break;
                                }
                                if (ln[i] == '#') {
                                    /* If some moron places a # character immediately after FDT (FDT#....) - just copy the rest of the line */
                                    lnstate = LS_COPY_REST;
                                    fputc(ln[i], ot);
                                    break;
                                }
                                /* No more whitespace - Skip original DTB filename and substitute our own */
                                lnstate = LS_DTB_FN;
                                fputs("/dtb/amlogic/", ot);
                                fputs(global_dir_list.fname[choice-1], ot);
                                /* Fall trough to LS_DTB_FN */
                            case LS_DTB_FN:
                                if (!isspace(ln[i]) && !(ln[i] =='#')) break;  /* Part of original DTB filename - skip character */
                                
                                /* End of original DTB filename */
                                lnstate = LS_COPY_REST;
                                /* NOTE: Parser falls through to LS_COPY_REST when end of original DTB filename is detected */
                            case LS_COPY_REST:
                                fputc(ln[i], ot);
                                break;
                        }
                    }
                }
                fclose(in); fclose(ot);
				
                /* Safe replace: remove old, then rename new */
                remove(E_CONF);
                if (rename(E_TMP, E_CONF) == 0)
                    printf("Success: extlinux.conf updated to %s\n", global_dir_list.fname[choice-1]);
                else {
                    printf("Error: Could not rename temp file to extlinux.conf\n");
                }
            }
        }
    }
}

int main() {
    int in;

    remove(E_TMP);
    while(1) {
        printf("\n====================================\n");
        printf("  AML-S9XX ARMBIAN BOOT MANAGER \n");
        printf("====================================\n");
        printf("1) Select and apply u-boot binary - (copy selected u-boot version to /u-boot.ext)\n");
        printf("2) Select new Device Tree Blob (DTB) - (edit /extlinux/extlinux.conf dtb entry, replace current dtb file with the selected\n");
        printf("0) Exit\n");
        printf("\nChoice: ");
        
        scanf("%d", &in);
		
        switch(in) {
			case 0: return 0;
            case 1: do_uboot(); break;
            case 2: do_dtb(); break;
            default: printf("Invalid choice, try again.\n");
        }
    }
}
