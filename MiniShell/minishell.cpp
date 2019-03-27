#include "minishell.h"

//Trims the leading (left) spaces from a string in place
void ltrim_str(string& s) {
    s.erase(s.begin(), find_if_not(s.begin(), s.end(), [](char c){ return isspace(c);}));
}

//Trims trailing (right) spaces from a string in place
void rtrim_str(string& s) {
    s.erase(find_if_not(s.rbegin(), s.rend(), [](char c) {return isspace(c);}).base(), s.end());
}

//trims left and right spaces of a copy of the string and returns that copy
string trim_str(const string& s) {
    string str = s;
    if(str.empty()) return "";
    ltrim_str(str);
    rtrim_str(str);
    return str;
}

//Parses a commandline command and its argument and returns the command and its args
//"ls -l -a" -> {"ls", "-l -a"}
pair<string, string> parse_line(const string& line) {
    string s, b;
    stringstream sin(line);
    sin >> s;
    getline(sin, b);
    return {s,b};
}

//Determines whether or not a string represents a number or not, true if it is a number false otherwise
bool is_number(const string& s) {
    if(s.empty() || !isdigit(s[0]))return false;
    return (s.find_first_not_of("0123456789") == s.npos);
}

//MiniShell constructor, initializes variables, reads data from file, starts up shell
MiniShell::MiniShell() { 
    char buf[100];
    getcwd(buf, 100);
    this->wd = string(buf);
    if(this->shell_vars.find("PATH") == end(this->shell_vars)) shell_vars["PATH"] = "";
    this->read_data(); 
}

// MiniShell destuctor: - required for build
MiniShell::~MiniShell() { return; }

//Reads the history and shell variables from file
void MiniShell::read_data() {
    string cmd, var, val;
    string hist_file = (this->wd + "/hist"), var_file = (this->wd + "/shvar"); //sets the abs path for the files
    fstream hf(hist_file, ios::in | ios::out), vf(var_file, ios::in | ios::out);
    if (!hf.is_open() && !vf.is_open()) return; //if the files couldnt be open, return (first time running possibly)

    while(getline(hf, cmd)) this->_history.push_back(cmd); //read all the files from the history and store in _history
    hf.close(); //close history file
    //read all the shell variables and store in shell_vars
    while(getline(vf, var)) {
        tie(var, val) = parse_line(var);
        this->shell_vars[var] = trim_str(val); 
    }
    vf.close(); ///close shell_vars file
}


//Writes the history and shell data to the file to be read when shell is restarted at later time
void MiniShell::persist_data() {
     string hist_file = (this->wd + "/hist"), var_file = (this->wd + "/shvar");//sets the abs path for the files
    fstream hf(hist_file, ios::out), vf(var_file, ios::out);
    if (!hf.is_open() && !vf.is_open()) { // if the files cant be written to there is an error, return
        printf("cannot write to data files. check file path.\n");
        return;
    }

    for(auto cmd: this->_history) hf << cmd << endl; //write all the history commands to file
    hf.close(); //close history file

    rep_iter(it, this->shell_vars.begin(), this->shell_vars.end()) { //write all the shell variables to file
        vf << it->first << " " << it->second << endl;
    }
    vf.close(); //close shell_vars file
}


// On Exit, saves the data to file and then exit
void MiniShell::exit() { 
    this->persist_data();
    if(!this->shell_vars.empty()) this->shell_vars.clear();
    if(!this->_history.empty()) this->_history.clear();
    cout << "exiting" << endl;
}

// MiniShell$ history : lists all the commands that were previously made since the program has been initiated
void MiniShell::list_history() {
    if(this->_history.empty()) return;
    int N = this->_history.size();
    for(auto i=0;i<N;i++) printf("%d %s\n", i+1, this->_history[i].c_str()); //displays all the commands and their order
}

//Stores all commands entered into the _history vector
void MiniShell::record_command(const string& cmd) {
    this->_history.push_back(cmd);
    this->persist_data(); 
}
//Parses the shell commands and their arguments, returns a pair of strings representing the shell var and its values
// "PATH=/bin" -> {"PATH", "/bin"}
pair<string, string> MiniShell::parse_shell_var(string var) {
    int N = var.length();
    if(N < 3) throw runtime_error("incorrect shell arguments"); //if the string has o
    auto pos = var.find("=");
    if(pos == string::npos || pos < 1 || pos == N-1) throw runtime_error("incorrect shell arguments");
    return {var.substr(0, pos), var.substr(pos+1, N-pos-1)};
}

//MiniShell$ export : Lists all the currently exported shell variables
void MiniShell::list_shell_vars() {
    if(this->shell_vars.empty()) return;
    rep_iter(it, this->shell_vars.begin(), this->shell_vars.end()) {
        printf("%s=%s\n", it->first.c_str(), it->second.c_str());
    }
}

//MiniShell$ export shell=/path/to/folder: Adds a new shell variable or appends a path to the end of a shell variable
//i.e PATH=/bin ; export PATH=/usr/bin -> PATH=/bin:/usr/bin
void MiniShell::add_shell_var(const string& var) {
    string s = trim_str(var), shell, val;
    try {
        tie(shell, val) = this->parse_shell_var(s); //parsing the shell can throw an error if the command doesnt match expected
        if(this->shell_vars.find(shell) == this->shell_vars.end()) { //if the shell var was never exported before
            this->shell_vars[shell] = val; //add it as a new shell
        } else {
            if(this->shell_vars[shell] != "") this->shell_vars[shell] += ":"+val; //append new path if it was 
            else this->shell_vars[shell] = val;
        }
    } catch (const exception& e) {
        printf("Incorrect use of export command.\n"); //if error display error message
    }
}
// Handles the export command, if there is no arguments, list all the shell vars, else add a new shell var
void MiniShell::export_variable(const string& var) {
    if(!var.empty()) this->add_shell_var(var);
    else this->list_shell_vars();
}

// Find all external commands that have been added to the PATH
void MiniShell::find_external_command(const string& ext_cmd) {
    if(ext_cmd.empty() || this->shell_vars.empty()) { //if there is not PATH, then this command doesnt exist
        printf("-MiniShell: %s: command not found.\n", ext_cmd.c_str());
        return;
    }
    stringstream ss(ext_cmd);
    string cmd, args;
    ss >> cmd; 
    rep_iter(it, this->shell_vars.begin(), this->shell_vars.end()) { //loop through all the shell variables in shell_vars
        string path;
        stringstream paths(it->second);
        while(getline(paths, path, ':')) { //get the paths of each shell variable, separated by ':'
            path = trim_str(path);
            DIR *pdir = opendir(path.c_str()); //open the path directory for each path 
            if(!pdir) continue; // if the path doesnt exist continue
            dirent *pdirent;

            while((pdirent = readdir(pdir)) != NULL ) { //if the path exists, open it and loop through the directory's contents
                if(strcmp(pdirent->d_name, cmd.c_str()) == 0) { //if the directory has a child with the same name, then it is a command
                    printf("%s is an external command (%s/%s)\n", cmd.c_str(), path.c_str(), cmd.c_str()); //display the commands abs path
                    if (!ss.eof()) printf("command arguments:\n"); //if the user specified arguments to the command
                    while(ss >> args) printf("%s\n", args.c_str()); // - display all the arguments
                    return; // return because we have found the command
                }
            }

        }
    }

    printf("-MiniShell: %s: command not found.\n", ext_cmd.c_str()); // if we have reached here, there is no command of that name
}

// MiniShell$ pwd : Prints the current working directory 
void MiniShell::pwd() {
    char buf[BUFF_SIZE];
    getcwd(buf, BUFF_SIZE);
    printf("%s\n", buf);
}

// MiniShell$ cd /path/to/folder : Changes the directory from the current working directory to the path specified
void MiniShell::cd(const string& path) {
    if(path.empty()) {
        printf("Please enter a path.\n");
        return;
    }
    if(chdir(path.c_str()) == -1) printf("Unable to change directory. Ensure '%s' is a valid path.\n", path.c_str());
}

// MiniShell$ !<cmd_num> : Executes a command in the history with the identifier <cmd_num>
string MiniShell::execute_history_command(const string& cmd_num) {
    if(!is_number(cmd_num)) return printf("'%s' is not a valid identifier.\n", cmd_num.c_str()), ""; //check if cmd_num is a valid 
    int index = stoi(cmd_num) - 1, n = this->_history.size(); //if the number is valid, convert to an int and subtract one to get the index
    if(index < 0 || index >= n) return printf("Command with identifier %s not found.\n", cmd_num.c_str()), "";
    return this->_history[index]; // if the index is valid (within the range of _history) then return that commmand to be executed
}

// Determines what command should be executed and passes the necessary arguments
void MiniShell::execute_command(const CommandType& cmd, const string& arguments) {
    string args = trim_str(arguments);
    switch(cmd) {
    case EXT: // if the command wasnt found, assume its an external command and try to find it
        this->find_external_command(args);
        break;
    case REC_CMD: //record the command entered
        this->record_command(args); 
        break;
    case CD: //change the directory
        this->cd(args);
        break;
    case PWD: //prints the workingdirectory
        this->pwd();
        break;
    case EXP: //execute the export command
        this->export_variable(args);
        break;
    case HIST: //lists the history
        this->list_history();
        break;
    case EXCL://execute a history command
        this->execute_history_command(args);
        break;
    default:
        break;
    }
}


