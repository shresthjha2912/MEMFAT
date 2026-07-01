#include "diskutils.h"

int main() {
    joindisk();

    cout << "+++ Number of blocks = "                   << NBLOCKS     << "\n";
    cout << "+++ Number of free blocks = "              << NFREEBLOCKS << "\n";
    cout << "+++ First block of the root directory = "  << RBN         << "\n";

    int    cbn = RBN;
    string cwd = "/";
    string line;

    while (true) {
        cout << "[foosh] VD:" << cwd << "> ";
        cout.flush();
        if (!getline(cin, line)) break;

        /* Tokenise */
        vector<string> args;
        string cur;
        for (char c : line) {
            if (c == ' ' || c == '\t') {
                if (!cur.empty()) { args.push_back(cur); cur.clear(); }
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) args.push_back(cur);
        if (args.empty()) continue;

        string cmd = args[0];

        if (cmd == "exit" || cmd == "quit") {
            break;

        } else if (cmd == "md" || cmd == "mkdir") {
            if (args.size() < 2) { cout << "Usage: md <path>\n"; continue; }
            md_cmd(args[1], cbn);

        } else if (cmd == "cd" || cmd == "chdir") {
            string p = (args.size() > 1) ? args[1] : "/";
            cbn = cd_cmd(p, cbn, cwd);

        } else if (cmd == "dir") {
            ls_cmd(".", cbn, false);

        } else if (cmd == "ls") {
            string p = (args.size() > 1) ? args[1] : ".";
            ls_cmd(p, cbn, true);

        } else if (cmd == "cp" || cmd == "copy") {
            if (args.size() < 3) { cout << "Usage: cp <src> <dest>\n"; continue; }
            cp_cmd(args[1], args[2], cbn);

        } else if (cmd == "prn" || cmd == "type") {
            if (args.size() < 2) { cout << "Usage: prn <file>\n"; continue; }
            prn_cmd(args[1], cbn);

        } else {
            cout << "*** Unknown command: " << cmd << "\n";
        }
    }

    memcpy(&NFREEBLOCKS, D + 4, 4);
    cout << "+++ Number of free blocks = " << NFREEBLOCKS << "\n";
    leavedisk();
    return 0;
}