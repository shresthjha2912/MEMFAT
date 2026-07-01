#include "diskutils.h"

char *D        = nullptr;
int            NBLOCKS      = 0;
int            NFREEBLOCKS  = 0;
int            RBN          = 0;
int            shmid        = -1;

void joindisk() {
    key_t key = ftok(".", 'A');
    if (key == -1) key = 12345;
    shmid = shmget(key, 67108864, 0666);
    if (shmid < 0) { perror("shmget"); exit(1); }
    D = (char *)shmat(shmid, NULL, 0);
    if (D == (char *)-1) { perror("shmat"); exit(1); }
    memcpy(&NBLOCKS,     D,     4);
    memcpy(&NFREEBLOCKS, D + 4, 4);
    memcpy(&RBN,         D + 8, 4);
}

void leavedisk() {
    if (D) { shmdt(D); D = nullptr; }
}

int get_bitmap(int blk) {
    int byte_idx = 1024 + (blk / 8);
    int bit_idx  = blk % 8;
    return (D[byte_idx] & (1 << bit_idx)) ? 1 : 0;
}

void set_bitmap(int blk, int val) {
    int byte_idx = 1024 + (blk / 8);
    int bit_idx  = blk % 8;
    if (val) D[byte_idx] |=  (1 << bit_idx);
    else     D[byte_idx] &= ~(1 << bit_idx);
}

int get_fat(int blk) {
    int val;
    memcpy(&val, D + 9216 + (blk * 4), 4);
    return val;
}

void set_fat(int blk, int next) {
    memcpy(D + 9216 + (blk * 4), &next, 4);
}

int getfreeblock() {
    memcpy(&NFREEBLOCKS, D + 4, 4);
    if (NFREEBLOCKS == 0) return 0;

    int blk;
    int attempts = 0;
    do {
        blk = rand() % 65536;
        attempts++;
        if (attempts > 65536 * 2) return 0; 
    } while (get_bitmap(blk) != 0);

    set_bitmap(blk, 1);
    NFREEBLOCKS--;
    memcpy(D + 4, &NFREEBLOCKS, 4);
    return blk;
}

void freeblock(int blk) {
    memcpy(&NFREEBLOCKS, D + 4, 4);
    int curr = blk;
    while (curr != 0 && curr != -1) {
        set_bitmap(curr, 0);
        NFREEBLOCKS++;
        int next = get_fat(curr);
        set_fat(curr, 0);
        curr = next;
    }
    memcpy(D + 4, &NFREEBLOCKS, 4);
}

vector<string> split_path(const string &path) {
    vector<string> tokens;
    string token;
    for (char c : path) {
        if (c == '/') {
            if (!token.empty()) { tokens.push_back(token); token.clear(); }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

int find_in_dir(int dir_blk, const string &name, metadata &out_meta) {
    int curr = dir_blk;
    while (curr != 0 && curr != -1) {
        for (int i = 0; i < 32; i++) {
            metadata tmp;
            memcpy(&tmp, D + (curr * 1024) + (i * 32), 32);
            if (tmp.type == 'f' || tmp.type == 'd') {
                if (string(tmp.name) == name) {
                    out_meta = tmp;
                    return curr;
                }
            }
        }
        curr = get_fat(curr);
    }
    return 0;
}

int resolve_path(const string &path, int current_dir_block,
                 string &filename, metadata &out_meta) {
    int curr = ((!path.empty() && path[0] == '/') ? RBN : current_dir_block);
    vector<string> tokens = split_path(path);

    if (tokens.empty()) {
        filename = ".";
        find_in_dir(curr, ".", out_meta);
        return curr;
    }

    for (int i = 0; i < (int)tokens.size(); i++) {
        int found = find_in_dir(curr, tokens[i], out_meta);
        if (!found) return 0;
        if (i < (int)tokens.size() - 1) {
            if (out_meta.type != 'd') return 0;
            curr = out_meta.firstblock;
        }
    }
    filename = tokens.back();
    return curr; 
}

void add_to_dir(int dir_blk, const metadata &meta) {
    int curr = dir_blk;
    int last = dir_blk;

    while (curr != 0 && curr != -1) {
        for (int i = 0; i < 32; i++) {
            metadata tmp;
            memcpy(&tmp, D + (curr * 1024) + (i * 32), 32);
            if (tmp.type != 'f' && tmp.type != 'd') {
                memcpy(D + (curr * 1024) + (i * 32), &meta, 32);
                metadata dot;
                memcpy(&dot, D + (dir_blk * 1024), 32);
                dot.size++;
                memcpy(D + (dir_blk * 1024), &dot, 32);
                return;
            }
        }
        last = curr;
        curr = get_fat(curr);
    }

    int new_blk = getfreeblock();
    if (!new_blk) { cerr << "***Error: disk full\n"; return; }
    set_fat(last, new_blk);
    set_fat(new_blk, -1);
    memset(D + (new_blk * 1024), 0, 1024);
    memcpy(D + (new_blk * 1024), &meta, 32);

    metadata dot;
    memcpy(&dot, D + (dir_blk * 1024), 32);
    dot.size++;
    memcpy(D + (dir_blk * 1024), &dot, 32);
}

void remove_from_dir(int dir_blk, const string &name) {
    int curr = dir_blk;
    while (curr != 0 && curr != -1) {
        for (int i = 0; i < 32; i++) {
            metadata tmp;
            memcpy(&tmp, D + (curr * 1024) + (i * 32), 32);
            if ((tmp.type == 'f' || tmp.type == 'd') && string(tmp.name) == name) {
                tmp.type = 0;
                memcpy(D + (curr * 1024) + (i * 32), &tmp, 32);
                metadata dot;
                memcpy(&dot, D + (dir_blk * 1024), 32);
                dot.size--;
                memcpy(D + (dir_blk * 1024), &dot, 32);
                return;
            }
        }
        curr = get_fat(curr);
    }
}

void md_cmd(string path, int cbn) {
    vector<string> tokens = split_path(path);
    if (tokens.empty()) return;

    int parent_blk = ((!path.empty() && path[0] == '/') ? RBN : cbn);

    for (int i = 0; i < (int)tokens.size(); i++) {
        metadata m;
        memset(&m, 0, 32);
        int found = find_in_dir(parent_blk, tokens[i], m);

        if (found) {
            if (m.type != 'd') {
                cout << "*** Error: " << tokens[i] << " is not a directory\n";
                return;
            }
            parent_blk = m.firstblock;
        } else {
            int n_blk = getfreeblock();
            if (!n_blk) { cerr << "***Error: disk full\n"; return; }
            set_fat(n_blk, -1);
            memset(D + (n_blk * 1024), 0, 1024);

            metadata new_entry;
            memset(&new_entry, 0, 32);
            new_entry.type = 'd';
            strncpy(new_entry.name, tokens[i].c_str(), 22);
            new_entry.name[22] = '\0';
            new_entry.size       = 2;
            new_entry.firstblock = n_blk;
            add_to_dir(parent_blk, new_entry);

            metadata self_e, par_e;
            memset(&self_e, 0, 32);
            memset(&par_e,  0, 32);
            self_e.type = 'd'; strncpy(self_e.name, ".",  23); self_e.size = 2; self_e.firstblock = n_blk;
            par_e.type  = 'd'; strncpy(par_e.name,  "..", 23); par_e.size  = 0; par_e.firstblock  = parent_blk;

            memcpy(D + (n_blk * 1024),      &self_e, 32);
            memcpy(D + (n_blk * 1024) + 32, &par_e,  32);

            parent_blk = n_blk;
        }
    }
}

void ls_cmd(string path, int cbn, bool detailed) {
    string fname;
    metadata m;
    memset(&m, 0, 32);
    int tgt = cbn;

    if (path.empty() || path == ".") {
        find_in_dir(cbn, ".", m);
        tgt = cbn;
    } else {
        if (!resolve_path(path, cbn, fname, m)) {
            cout << "*** Error: path not found: " << path << "\n";
            return;
        }
        if (m.type == 'f') {
            if (detailed)
                cout << m.type << "\t" << m.name << "\t" << m.size << "\t" << m.firstblock << "\n";
            else
                cout << m.name << "\n";
            return;
        }
        tgt = m.firstblock;
        find_in_dir(tgt, ".", m);
    }

    if (detailed) {
        cout << "Total " << m.size << " entries\n";
        cout << "-----------------------------------------------------------------------\n";
        cout << "TYPE\tNAME\tSIZE\tFIRST BLOCK\n";
        cout << "-----------------------------------------------------------------------\n";
    }

    int curr = tgt;
    while (curr != 0 && curr != -1) {
        for (int i = 0; i < 32; i++) {
            metadata tmp;
            memcpy(&tmp, D + (curr * 1024) + (i * 32), 32);
            if (tmp.type == 'f' || tmp.type == 'd') {
                if (detailed) {
                    cout << tmp.type << "\t"
                         << tmp.name << (tmp.type == 'd' ? "/" : "") << "\t"
                         << tmp.size << "\t"
                         << tmp.firstblock << "\n";
                } else {
                    cout << tmp.name << (tmp.type == 'd' ? "/" : "") << " ";
                }
            }
        }
        curr = get_fat(curr);
    }
    if (detailed)
        cout << "-----------------------------------------------------------------------\n";
    else
        cout << "\n";
}

int cd_cmd(string path, int cbn, string &cwd_str) {
    if (path.empty() || path == "/") {
        cwd_str = "/";
        return RBN;
    }

    string fname;
    metadata m;
    if (!resolve_path(path, cbn, fname, m) || m.type != 'd') {
        cout << "*** Error: unable to change to directory " << path << "\n";
        return cbn;
    }

    if (!path.empty() && path[0] == '/') {
        string new_cwd = "/";
        vector<string> tokens = split_path(path);
        for (int i = 0; i < (int)tokens.size(); i++) {
            if (tokens[i] == ".") continue;
            if (tokens[i] == "..") {
                size_t pos = new_cwd.find_last_of('/');
                new_cwd = (pos == 0) ? "/" : new_cwd.substr(0, pos);
            } else {
                if (new_cwd.back() != '/') new_cwd += '/';
                new_cwd += tokens[i];
            }
        }
        cwd_str = new_cwd;
    } else {
        vector<string> tokens = split_path(path);
        for (int i = 0; i < (int)tokens.size(); i++) {
            if (tokens[i] == ".") continue;
            if (tokens[i] == "..") {
                size_t pos = cwd_str.find_last_of('/');
                cwd_str = (pos == 0) ? "/" : cwd_str.substr(0, pos);
            } else {
                if (cwd_str.back() != '/') cwd_str += '/';
                cwd_str += tokens[i];
            }
        }
    }

    return m.firstblock;
}

int resolve_dest(const string &dest, int cbn,
                        string &out_fname, bool &dest_is_dir) {
    string fname;
    metadata m;
    memset(&m, 0, 32);
    int found = resolve_path(dest, cbn, fname, m);

    if (found && m.type == 'd') {
        dest_is_dir = true;
        out_fname   = "";         
        return m.firstblock;
    }
    dest_is_dir = false;
    if (found && m.type == 'f') {
        out_fname = fname;
        size_t last = dest.find_last_of('/');
        if (last == string::npos) {
            return cbn;
        }
        string p_path = dest.substr(0, last);
        string df; metadata pm;
        if (!resolve_path(p_path, cbn, df, pm) || pm.type != 'd') {
            cout << "*** Error: destination directory not found\n";
            return 0;
        }
        return pm.firstblock;
    }
    size_t last = dest.find_last_of('/');
    if (last == string::npos) {
        out_fname = dest;
        return cbn;
    }
    string p_path = dest.substr(0, last);
    out_fname     = dest.substr(last + 1);
    string df; metadata pm;
    if (!resolve_path(p_path, cbn, df, pm) || pm.type != 'd') {
        cout << "*** Error: destination directory not found\n";
        return 0;
    }
    return pm.firstblock;
}

void cp_cmd(string src, string dest, int cbn) {
    bool src_hd  = (!src.empty()  && src[0]  == '`');
    bool dest_hd = (!dest.empty() && dest[0] == '`');
    if (src_hd)  src  = src.substr(1);
    if (dest_hd) dest = dest.substr(1);

    if (src_hd && !dest_hd) {
        FILE *f = fopen(src.c_str(), "rb");
        if (!f) {
            cout << "*** Error: Unable to read input file " << src << "\n";
            return;
        }
        fseek(f, 0, SEEK_END);
        int sz = (int)ftell(f);
        rewind(f);

        string fname;
        bool dest_is_dir = false;
        int p_dir = resolve_dest(dest, cbn, fname, dest_is_dir);
        if (!p_dir) { fclose(f); return; }

        if (dest_is_dir) {
            size_t pos = src.find_last_of('/');
            fname = (pos == string::npos) ? src : src.substr(pos + 1);
        }

        metadata existing;
        if (find_in_dir(p_dir, fname, existing)) {
            freeblock(existing.firstblock);
            remove_from_dir(p_dir, fname);
        }

        int prev = 0, first = 0, rem = sz;
        while (rem > 0) {
            int blk = getfreeblock();
            if (!blk) { cout << "*** Error: disk full\n"; fclose(f); return; }
            if (!first) first = blk;
            if (prev) set_fat(prev, blk);
            set_fat(blk, -1);
            int chunk = (rem > 1024) ? 1024 : rem;
            fread(D + (blk * 1024), 1, chunk, f);
            rem -= chunk;
            prev = blk;
        }
        fclose(f);

        metadata nm;
        memset(&nm, 0, 32);
        nm.type = 'f';
        strncpy(nm.name, fname.c_str(), 22);
        nm.name[22]   = '\0';
        nm.size       = sz;
        nm.firstblock = (sz > 0) ? first : -1;
        add_to_dir(p_dir, nm);

    } else if (!src_hd && dest_hd) {
        string fname;
        metadata m;
        if (!resolve_path(src, cbn, fname, m) || m.type != 'f') {
            cout << "*** Error: source file not found: " << src << "\n";
            return;
        }
        FILE *f = fopen(dest.c_str(), "wb");
        if (!f) {
            cout << "*** Error: cannot create file " << dest << "\n";
            return;
        }
        int curr = m.firstblock, rem = m.size;
        while (curr != 0 && curr != -1 && rem > 0) {
            int chunk = (rem > 1024) ? 1024 : rem;
            fwrite(D + (curr * 1024), 1, chunk, f);
            rem -= chunk;
            curr = get_fat(curr);
        }
        fclose(f);

    } else if (!src_hd && !dest_hd) {
        string src_fname;
        metadata sm;
        if (!resolve_path(src, cbn, src_fname, sm) || sm.type != 'f') {
            cout << "*** Error: source file not found: " << src << "\n";
            return;
        }

        string dst_fname;
        bool dest_is_dir = false;
        int p_dir = resolve_dest(dest, cbn, dst_fname, dest_is_dir);
        if (!p_dir) return;

        if (dest_is_dir) dst_fname = src_fname;

        metadata existing;
        if (find_in_dir(p_dir, dst_fname, existing)) {
            freeblock(existing.firstblock);
            remove_from_dir(p_dir, dst_fname);
        }

        int prev = 0, first = 0, rem = sm.size;
        int src_curr = sm.firstblock;
        while (src_curr != 0 && src_curr != -1 && rem > 0) {
            int blk = getfreeblock();
            if (!blk) { cout << "*** Error: disk full\n"; return; }
            if (!first) first = blk;
            if (prev) set_fat(prev, blk);
            set_fat(blk, -1);
            int chunk = (rem > 1024) ? 1024 : rem;
            memcpy(D + (blk * 1024), D + (src_curr * 1024), chunk);
            rem -= chunk;
            prev = blk;
            src_curr = get_fat(src_curr);
        }

        metadata nm;
        memset(&nm, 0, 32);
        nm.type = 'f';
        strncpy(nm.name, dst_fname.c_str(), 22);
        nm.name[22]   = '\0';
        nm.size       = sm.size;
        nm.firstblock = (sm.size > 0) ? first : -1;
        add_to_dir(p_dir, nm);

    } else {
        cout << "*** Error: HD-to-HD copy not supported\n";
    }
}

void prn_cmd(string path, int cbn) {
    string fname;
    metadata m;
    if (!resolve_path(path, cbn, fname, m) || m.type != 'f') {
        cout << "*** Error: file not found: " << path << "\n";
        return;
    }
    int curr = m.firstblock, rem = m.size;
    while (curr != 0 && curr != -1 && rem > 0) {
        int chunk = (rem > 1024) ? 1024 : rem;
        for (int i = 0; i < chunk; i++) cout << (char)D[(curr * 1024) + i];
        rem -= chunk;
        curr = get_fat(curr);
    }
    cout << "\n";
}