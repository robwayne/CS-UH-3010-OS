#include "minishell.h"

int main() {
    auto simp_shell = MiniShell(); //start the  shell
    string line, cmd, args;
    while(1) {
        cout << ">> ";
        getline(cin, line);
        tie(cmd, args) = parse_line(line); //parse the command line


        if (cmd[0] == '!') { //execute history command
            line = simp_shell.execute_history_command(cmd.substr(1,cmd.length()-1));
            if (line == "") continue;
            tie(cmd, args) = parse_line(line);
        }

        if(cmd == "exit") { //exit the shell
            simp_shell.execute_command(REC_CMD, line);
            break;
        }

        if(cmd == "cd") simp_shell.execute_command(CD, args);
        else if (cmd == "history") simp_shell.execute_command(HIST);
        else if (cmd == "export") simp_shell.execute_command(EXP, args);
        else if (cmd == "pwd") simp_shell.execute_command(PWD);
        else simp_shell.execute_command(EXT, line);

        simp_shell.execute_command(REC_CMD, line);
    }
    return 0;
}