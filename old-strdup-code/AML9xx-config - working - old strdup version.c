/* 
 * AML-S9XX ARMBIAN BOOT CONFIGURATION TOOL
 * ----------------------------------------
 * 
 * SIZE-OPTIMIZED COMPILATION GUIDE:
 *
 * 1. WINDOWS (MinGW-w64):
 *    gcc -Os -s -static -ffunction-sections -fdata-sections -fno-ident \
 *        -fno-asynchronous-unwind-tables -Wl,--gc-sections \
 *        aml-config.c -o aml-config-win-x64.exe
 *
 * 2. LINUX ARM64 (Native on Amlogic box):
 *    musl-gcc -Os -s -static -ffunction-sections -fdata-sections \
 *             -Wl,--gc-sections aml-config.c -o aml-config-arm64
 *
 * 3. LINUX x86_64 (Standard PC):
 *    musl-gcc -Os -s -static -ffunction-sections -fdata-sections \
 *             -Wl,--gc-sections aml-config.c -o aml-config-linux-x64
 */

#include <stdio.h>
#include <stdlib.h>
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
    FILE *s = fopen(src, "rb");
    FILE *d = fopen(dst, "wb");
    unsigned char b;
    if (!s || !d) { if(s) fclose(s); return; }
    while (fread(&b, 1, 1, s)) fwrite(&b, 1, 1, d);
    fclose(s); fclose(d);
}

void do_uboot() {
    struct dirent *e;
    char *fls[128];
    int c = 0, i, ch;
    DIR *d = opendir(".");
    if (!d) return;

    printf("\n--- SELECT U-BOOT BINARY ---\n");
    printf("Scanning root directory for u-boot-s9xx files...\n");

    while ((e = readdir(d)) != NULL && c < 128) {
        if (strncmp(e->d_name, "u-boot-s9", 9) == 0) {
            fls[c] = strdup(e->d_name);
            printf("%d) %s\n", ++c, e->d_name);
        }
    }
    closedir(d);

    if (c > 0) {
        printf("0) Cancel and return\n");
        printf("Choice (1-%d): ", c);
        if (scanf("%d", &ch) == 1 && ch > 0 && ch <= c) {
            printf("Applying %s as %s...\n", fls[ch-1], U_DEST);
            f_copy(fls[ch-1], U_DEST);
            printf("Success!\n");
        }
        for(i=0; i<c; i++) free(fls[i]);
    } else {
        printf("No compatible u-boot files found.\n");
    }
}

void do_dtb() {
    struct dirent *e;
    unsigned char *fls[256];
    int c = 0, i, ch;
    unsigned char ln[1024];

    if (!f_exists(E_BAK)) {
        if (f_exists(E_CONF)) {
            printf("First run: Backing up original extlinux.conf...\n");
            f_copy(E_CONF, E_BAK);
        } else {
            printf("Error: %s not found!\n", E_CONF);
            return;
        }
    }

    DIR *d = opendir(D_DIR);
    if (!d) { printf("Error: DTB directory %s missing\n", D_DIR); return; }

    printf("\n--- SELECT DEVICE TREE (DTB) ---\n");
    while ((e = readdir(d)) != NULL && c < 256) {
        if (strstr(e->d_name, ".dtb")) {
            fls[c] = strdup(e->d_name);
            printf("%3d) %s\n", ++c, e->d_name);
        }
    }
    closedir(d);

    if (c > 0) {
        printf("  0) Cancel and return\n");
        printf("Choice (1-%d): ", c);
        if (scanf("%d", &ch) == 1 && ch > 0 && ch <= c) {
            FILE *in = fopen(E_CONF, "r");
            FILE *ot = fopen(E_TMP, "w");
            if(in && ot) {
                while (fgets(ln, sizeof(ln), in)) {
                    if (ln[strspn(ln, " \t")] == '#') /* comment - copy line */
                        fputs(ln, ot);
                    else {
                        i = strspn(ln, " \t");  /* Skip whitespace before comparing for fdt */
                        if ((strlen(ln)>=i+3) && (toupper(ln[i]) == 'F') && (toupper(ln[i+1]) == 'D') && (toupper(ln[i+2]) == 'T') && isspace(ln[i+3])) {
                            ln[i+3] = '\0';
                            fprintf(ot, "%s /dtb/amlogic/%s\n", ln, fls[ch-1]);
                        }
                        else {
                            fputs(ln, ot);
                        }
                    }
                }
                fclose(in); fclose(ot);
				
                /* Safe replace: remove old, then rename new */
                remove(E_CONF);
                if (rename(E_TMP, E_CONF) == 0)
                    printf("Success: extlinux.conf updated to %s\n", fls[ch-1]);
                else {
                    printf("Error: Could not rename temp file to extlinux.conf\n");
                }
            }
        }
        for(i=0; i<c; i++) free(fls[i]);
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
        printf("2) Select and apply Device Tree (DTB) - (edit /extlinux/extlinux.conf dtb line to match the name of the selected dtb file)\n");
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
