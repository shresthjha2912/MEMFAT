#include "diskutils.h"
#define VD_SIZE    67108864   
#define TOTAL_BLKS 65536
#define ROOT_BLK   265
#define FAT_EOF    -1  

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " create|remove\n";
        return 1;
    }

    key_t key = ftok(".", 'A');
    if (key == -1) key = 12345;

    string mode = argv[1];

    if (mode == "create") {
        int shmid = shmget(key, VD_SIZE, IPC_CREAT | IPC_EXCL | 0666);
        if (shmid < 0) {
            shmid = shmget(key, VD_SIZE, 0666);
            if (shmid >= 0) shmctl(shmid, IPC_RMID, NULL);
            shmid = shmget(key, VD_SIZE, IPC_CREAT | IPC_EXCL | 0666);
            if (shmid < 0) { perror("shmget"); return 1; }
        }

            char *D = (char *)shmat(shmid, NULL, 0);
        if (D == (char *)-1) { perror("shmat"); return 1; }

        memset(D, 0, VD_SIZE);

        int nblocks     = TOTAL_BLKS;
        int nfreeblocks = TOTAL_BLKS - ROOT_BLK - 1; 
        int rbn         = ROOT_BLK;

        memcpy(D,     &nblocks,     4);
        memcpy(D + 4, &nfreeblocks, 4);
        memcpy(D + 8, &rbn,         4);

  
        for (int blk = 0; blk <= ROOT_BLK; blk++) {
            int byte_idx = 1024 + (blk / 8);
            int bit_idx  = blk % 8;
            D[byte_idx] |= (1 << bit_idx);
        }

        int fat_eof = FAT_EOF;
        memcpy(D + 9216 + (ROOT_BLK * 4), &fat_eof, 4);

        metadata self_entry, par_entry;
        memset(&self_entry, 0, 32);
        memset(&par_entry,  0, 32);

        self_entry.type = 'd';
        strncpy(self_entry.name, ".", 23);
        self_entry.size       = 2; 
        self_entry.firstblock = ROOT_BLK;

        par_entry.type = 'd';
        strncpy(par_entry.name, "..", 23);
        par_entry.size       = 0;
        par_entry.firstblock = ROOT_BLK;

        memcpy(D + (ROOT_BLK * 1024),      &self_entry, 32);
        memcpy(D + (ROOT_BLK * 1024) + 32, &par_entry,  32);

        shmdt(D);
        cout << "+++ Virtual disk created with " << TOTAL_BLKS << " blocks\n";

    } else if (mode == "remove") {
        int shmid = shmget(key, VD_SIZE, 0666);
        if (shmid < 0) { perror("shmget"); return 1; }
        if (shmctl(shmid, IPC_RMID, NULL) < 0) { perror("shmctl"); return 1; }
        cout << "+++ Virtual disk removed\n";

    } else {
        cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}