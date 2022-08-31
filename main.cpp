// Mayank Gupta - 2022201012 - OS1 - @IIIT Hyderabad

#include <bits/stdc++.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

#define maxRowsInNormalMode (windowStartIndex + windowRows - 5)

// #define cursorSelected "==> "
#define cursorSelected ">>> "
#define cursorUnselected "    "

using namespace std;

string homeDirectory;
string currentWorkingDirectory;
vector<vector<string>> dirInfo; // For Storing directory info (including sub-directory & Files)
struct termios original_termios; // Store original state of terminal
bool normalMode = true; // set to True if normal mode is on, else false

stack<string> backwardStack; // Maintains Backward History
stack<string> forwardStack; // Maintains forward History
int windowRows, windowCols; // Terminal size
int windowStartIndex = 0; // First Line of Terminal 
int rowIndex = 0; // cursor pointing to this index.

string inputCommandString = "";

void handleKeyPressesInCommandMode();
void printDirInfo(string path);

// Clears Forward Stack
void clearForwardStack() {
    while (!forwardStack.empty()) {
        forwardStack.pop();
    }
}

// Clears the Screen.
void clearScreen() {
    cout << "\033[H\033[2J\033[3J"; // ANSI escape Sequence to clear screen.
}

// Prints the Error & Exit from the Programme
void printError(string s) {
  cout << "Error : " << s << "\r\n";
  exit(1);
}

// Convert Seconds into date/time;
string GetTimeAndDate(unsigned long long sec) {
    char date[100];
    time_t seconds = (time_t)(sec);
    strftime(date, sizeof(date) - 1, "%d-%b-%Y %R ", localtime(&seconds));
    struct tm *tmmm = localtime(&seconds);

    return date;
}

// Convert fileSize from Bytes to B/KB/MB/GB/TB
string convertSize(unsigned long int fileSize) {
    unsigned long int kb = 1024;    //KB
    unsigned long int mb = kb * kb; //MB
    unsigned long int gb = mb * mb; //GB

    string convertedSize = "";
    if(fileSize >= gb) {
       convertedSize = to_string(llround(fileSize/gb)) + "GB";
    }
    else if(fileSize >= mb) {
       convertedSize = to_string(llround(fileSize/mb)) + "MB";
    }
    else if(fileSize >= kb) {
       convertedSize = to_string(llround(fileSize/kb)) + "KB";
    }
    else {
        convertedSize = to_string(fileSize) + "B";
    }

    return convertedSize;
}

// Check If given path is a directory
bool isDirectory(string path) {
    struct stat fileStat;
    stat(path.c_str(),&fileStat);
    return S_ISDIR(fileStat.st_mode);
}

// Fetching Current System User, and storing it in global variable systemUserName
void getHomeDirectory() {
    homeDirectory = "/home/" + (string)getpwuid(getuid())->pw_name + "/";
}

// for fetching File Info stats like git status(fileSize , username, groupname, last modified date, file permission)
vector<string> getFileInfo(string fileName, string filePath) {
    vector<string> fileInfo;

    string fullFileName = filePath + fileName;

    struct stat fileStat;
    if(stat(fullFileName.c_str(),&fileStat) < 0)   {
        cout << "File Not Exists \n";
        return {};
    }
    // cout << to_string(fileStat.st_size) << '\t';
    string fileSize = convertSize(fileStat.st_size);
    string fileUserName = getpwuid(fileStat.st_uid)->pw_name;
    string fileGroupName = getgrgid(fileStat.st_gid)->gr_name;

    string filePermission = ""; // To store permission 
    filePermission += (S_ISDIR(fileStat.st_mode)) ? 'd' : '-'; // If directory than 'd', else '-'
    filePermission += (fileStat.st_mode & S_IRUSR) ? 'r' : '-'; // If user has read permission, than 'r'
    filePermission += (fileStat.st_mode & S_IWUSR) ? 'w' : '-'; // If user has write permission, than 'w'
    filePermission += (fileStat.st_mode & S_IXUSR) ? 'x' : '-'; // If user has execute permission, than 'x'
    filePermission += (fileStat.st_mode & S_IRGRP) ? 'r' : '-'; // If group has read permission, than 'r'
    filePermission += (fileStat.st_mode & S_IWGRP) ? 'w' : '-'; // If group has write permission, than 'w'
    filePermission += (fileStat.st_mode & S_IXGRP) ? 'x' : '-'; // If group has execute permission, than 'x'
    filePermission += (fileStat.st_mode & S_IROTH) ? 'r' : '-'; // If other has read permission, than 'r'
    filePermission += (fileStat.st_mode & S_IWOTH) ? 'w' : '-'; // If other has write permission, than 'w'
    filePermission += (fileStat.st_mode & S_IXOTH) ? 'x' : '-'; // If other has execute permission, than 'x'
    
    string fileLastModifiedDate = GetTimeAndDate(fileStat.st_mtim.tv_sec); //last modified time

    fileInfo.push_back(fileName);               //fileInfo[0] = fileName
    fileInfo.push_back(filePath);               //fileInfo[1] = filePath
    fileInfo.push_back(fileSize);               //fileInfo[2] = fileSize
    fileInfo.push_back(fileUserName);           //fileInfo[3] = fileUserName
    fileInfo.push_back(fileGroupName);          //fileInfo[4] = fileGroupName
    fileInfo.push_back(filePermission);         //fileInfo[5] = filePermission
    fileInfo.push_back(fileLastModifiedDate);   //fileInfo[6] = fileLastModifiedDate

    return fileInfo;
}

// Stores the directory/Files info into global vector dirInfo.
void getDirectoryInfo(string path) {
    dirInfo.clear();
    // cout << path.c_str() << "\n";
    vector<string> fileNames; // For storing names of files & sub-directories
    struct dirent *de;
    DIR *dr = opendir(path.c_str());
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        // currentWorkingDirectory = backwardStack.top();
        // backwardStack.pop();
        // clearScreen();
        // printDirInfo("/");
        cout << "\033[35m" << "Can't Open File Directory. Press Backspace for Parent Directory / Left Space for Previous Directory.\r\n" << "\033[0m";
        return;
    }
     while ((de = readdir(dr)) != NULL){
            fileNames.push_back(de->d_name);
            // cout << de->d_name << '\t';
            // cout << _D_EXACT_NAMLEN(de) << '\t';
    }
    closedir(dr);

    sort(fileNames.begin() , fileNames.end());

    for(string fileName : fileNames) {

        vector<string> fileInfo = getFileInfo(fileName, path);
        dirInfo.push_back(fileInfo);
        // cout << v[1] << "\t" << v[2] << "\t" << v[3] << "\t" << v[4] << "\t" << v[5] << "\t" << v[6] << "\t" << v[0] << "\n";
        // cout<<"\033[21;35m"<<v[0]<<"\033[0m" << "  " << v[1] << "\n";
    }
  
}   

// Returns Parent Directory
string getParentDirectory(string path) {
    vector<string> strArr;
    stringstream ss (path);
    string item;
    while (getline (ss, item, '/')) {
        if(item == "") continue;
        strArr.push_back (item);
    }
    if(strArr.size() == 0 || strArr.size() == 1) {
        return "/";
    }
    else {
        string newDirectory = "/";
        for(int i = 0 ; i < strArr.size() - 1 ; i++) {
            newDirectory += strArr[i] + "/";
        }
        return newDirectory;
    }
    // cout << newDirectory << endl;
    // return "";
}

// Opens the file in default editor
void openFile(string filePath) {
    const char *fileName = filePath.c_str();
    pid_t pid = fork();
    if(pid == 0) {
        execlp("xdg-open", "xdg-open", fileName, (char *)0);
    }
}

// returns fileInfo in a string of size equals to window column size
string resizeFileInfo(vector<string> fileInfo) {
    // FileName
    string fileName = fileInfo[0];
    while(fileName.size() < 25) {
        fileName += ' ';
    }
    if(fileName.size() > 25) {
        fileName = fileName.substr(0, 22) + "..."; 
    }

    // FileSize
    string fileSize = fileInfo[2];
    while(fileSize.size() < 7) {
        fileSize = ' ' + fileSize;
    }

    // UserName
    string userName = fileInfo[3];
    while(userName.size() < 10) {
        userName += ' ';
    }
    if(userName.size() > 10) {
        userName = userName.substr(0, 7) + "..."; 
    }

    // GroupName
    string groupName = fileInfo[4];
    while(groupName.size() < 10) {
        groupName += ' ';
    }
    if(groupName.size() > 10) {
        groupName = groupName.substr(0, 7) + "..."; 
    }

    // File Permission
    string filePermission = fileInfo[5];

    // Last Modified Date
    string lastModifiedDate = fileInfo[6];
    
    string resizedFileInfo = fileName + "   " + fileSize + "   " + userName + "   " + groupName + "   " + filePermission + "   " + lastModifiedDate;
    resizedFileInfo = resizedFileInfo.substr(0,windowCols-5);

    return resizedFileInfo;
}

// Print Directory Info
void printDirInfo(string path) {
    clearScreen(); //Clearing the Screen before Printing.
    getDirectoryInfo(path);
    for(int dirIndex = windowStartIndex ; /*dirIndex < dirInfo.size() && */dirIndex < maxRowsInNormalMode; dirIndex++) {
        if(dirIndex >= dirInfo.size()) {
            cout << "\r\n";
            continue;
        }
        vector<string> fileInfo = dirInfo[dirIndex];
        if(normalMode == true && dirIndex == rowIndex) {
            cout<<"\033[1;7m";
            cout << cursorSelected << resizeFileInfo(fileInfo) << "\r\n";
            cout<<"\033[0m";
        }
        else {
            cout << cursorUnselected << resizeFileInfo(fileInfo) << "\r\n";
            // if(fileInfo[5][0] == 'd') cout<<"\033[32m";
            // else cout<<"\033[31m";
            // cout << fileInfo[0] << "\r\n";
            // cout<<"\033[0m";
        }
    }
    cout << "----------------------------------------" << "\r\n";
    // cout<<"\033[1;7m";
    if(normalMode) {
        cout << " > Normal Mode" << "\r\n";
    }
    else {
        cout << " > Command Mode" << "\r\n";
    }
    // cout<<"\033[0m";
    cout << currentWorkingDirectory;
    // cout.flush();
    if(normalMode == 0) cout << "$ " << inputCommandString ;
    cout.flush();
}

// Enables Raw Mode (Canonical Mode)
void disableRawMode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) printError("tcgetattr");
}

// Enables Raw Mode (Non-Canonical Mode)
void enableRawMode() {
    // tcflag_t c_iflag;		/* input mode flags */
    // tcflag_t c_oflag;		/* output mode flags */
    // tcflag_t c_cflag;		/* control mode flags */
    // tcflag_t c_lflag;		/* local mode flags */
    // cc_t c_line;			/* line discipline */
    // cc_t c_cc[NCCS];		/* control characters */
    // speed_t c_ispeed;		/* input speed */
    // speed_t c_ospeed;		/* output speed */
    if(tcgetattr(STDIN_FILENO, &original_termios) == -1) printError("tcgetattr");
    atexit(disableRawMode);

    struct termios raw_termios = original_termios;
    raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cflag |= (CS8);
    raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1) printError("tcsetattr");
}

// Remove extra '.' and '..' from the path.
string removeRedundancyFromPath(string path) {
    vector<string> strArr;
    stringstream ss (path);
    string item;
    while (getline (ss, item, '/')) {
        cout << item << " " << endl;
        if(item == "" || item == ".") continue;
        if(item == "..") {
            strArr.pop_back();
            continue;
        }
        strArr.push_back (item);
    }
    cout << endl << strArr.size() << endl;
    if(strArr.size() == 0) {
        return "/";
    }
    else {
        string newDirectory = "/";
        for(int i = 0 ; i < strArr.size(); i++) {
            newDirectory += strArr[i] + "/";
        }
        return newDirectory;
    }
}

// returns the absolute path, irrespetive of when the inputh path is relative or absolute.
string getAbsolutePath(string path) {
    string absolutePath = "";
    if(path[0] != '/') {
        absolutePath = currentWorkingDirectory + path;
    }
    else {
        absolutePath = path;
    }

    if(absolutePath[absolutePath.size() - 1] != '/') {
        absolutePath += '/';
    }
    // cout << "Hi : " << absolutePath << endl;
    absolutePath = removeRedundancyFromPath(absolutePath);
    // cout << "Hello : " << absolutePath << endl;
    cout << absolutePath << "\r\n";
    return absolutePath;
}

// Switch to any path.
void goToPath(string path) {
    string newPath = getAbsolutePath(path);
    currentWorkingDirectory = newPath;
    inputCommandString = "";
    printDirInfo(currentWorkingDirectory);
}

// Checks if the given file/directory is present in current working directory or not. (Need to check Recursively)
bool search(string filename, string path) {
    if(filename == "." || filename == "..") return true;
    struct dirent *de;
    DIR *dr = opendir(path.c_str());
    if (dr != NULL) {
            while ((de = readdir(dr)) != NULL) {
            string d_name = de->d_name;
            if(d_name == "." || d_name == "..") {
                continue;
            }
            if(d_name == filename) return true;
            string newPath = path + d_name + "/";
            // cout << newPath << endl;
            if(isDirectory(newPath)) {
                if(search(filename, newPath)) return true;
            }
        }
    }
    closedir(dr);
    return 0;
}

// Processing input from command and do desired operation.
void processBufferStringAndDoDesiredOperation() {
    vector<string> v;
    string temp = "";
    for(int i = 0 ; i < inputCommandString.size() ; i++) {
        if(inputCommandString[i] != ' ' && inputCommandString[i] != '\t') {
            temp += inputCommandString[i];
        }
        else if(temp != "") {
            v.push_back(temp);
            temp = "";
        }
    }
    if(temp != "") {
        v.push_back(temp);
        temp = "";
    }
    inputCommandString = "";

    if(v.size() == 0) {
        cout << "Invalid Input" << "\r\n";
    }
    
    // Quit
    else if(v[0] == "quit") {
        clearScreen();
        // cout << "Thanks for using File Explorer.\r\n";
        exit(1);
    }

    else if(v.size() == 1) {
        cout << "Invalid Input" << "\r\n";
    }

    // Goto
    else if(v[0] == "goto") {
        // cout << "Hi";
        if(v.size() == 2) {
            goToPath(v[1]);
            // cout << "goto\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 
    
    // Search
    else if(v[0] == "search") {
        if(v.size() == 2) {
            // cout << "search\r\n";
            if(search(v[1], currentWorkingDirectory)) {
                cout << "True" << "\r\n";
            } else {
                cout << "False" << "\r\n"; 
            }
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    // Create File
    else if(v[0] == "create_file") {
        if(v.size() == 2) {
            cout << "create_file\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    // Create Directory
    else if(v[0] == "create_dir") {
        if(v.size() == 2) {
            cout << "create_dir\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    // Delete File
    else if(v[0] == "delete_file") {
        if(v.size() == 2) {
            cout << "delete_file\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    // Delete Directory
    else if(v[0] == "delete_dir") {
        if(v.size() == 2) {
            cout << "delete_dir\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    // Rename
    else if(v[0] == "rename") {
        if(v.size() == 2) {
            cout << "rename\r\n";
        }
        else {
            cout << "Invalid Input" << "\r\n";
        }
    } 

    else if(v.size() == 2) {
        cout << "Invalid Input" << "\r\n";
    }

    // Copy
    else if(v[0] == "copy") {
        cout << "copy\r\n";
    }
    
    // Move
    else if(v[0] == "move") {
        cout << "copy\r\n";
    }

    else {
        cout << "Invalid Input" << "\r\n";
    }
    // cout << "\r\n";
    // for(int i = 0; i < v.size() ; i++) {
    //     cout << v[i] << "\r\n";
    // }
}

// Key Presses in command Mode.
void handleKeyPressesInCommandMode() {
    printDirInfo(currentWorkingDirectory);
    while(true) {
        char ch = '\0';
        read(STDIN_FILENO, &ch, 1);
        
        // Escape Key : Should switch to Normal Mode
        if(ch == 27) {
            normalMode = 1;
            inputCommandString = "";
            windowStartIndex = 0;
            rowIndex = 0;
            break;
        }

        // Enter Key - Process the Buffer & do desired opertaion.
        else if(ch == 13) {
            processBufferStringAndDoDesiredOperation();
            // cout << "Enter Key\r\n";
        }

        // Backspace : Remove character from Buffer (If available).
        else if(ch == 127) {
            if(inputCommandString.size() > 0) {
                inputCommandString.pop_back();
            }
            printDirInfo(currentWorkingDirectory);
        }

        // Add the input characters to Buffer.
        else {
            inputCommandString += ch;
            printDirInfo(currentWorkingDirectory);
        }
        
    }
    // cout << "\r\n\033[31m" << "Command Mode is in Implementation Phase." << "\033[0m";
    // cout.flush();
    // while(1);
}

// Handle Escape Key
int readEscape() {
    char finalChar = '\0';
    char ch1;
    read(STDIN_FILENO, &ch1, 1);
    if(ch1 == 27) return readEscape();
    if(ch1 == 91) {
        char ch2;
        read(STDIN_FILENO, &ch2, 1);
        if(ch2 == 27) return readEscape();
        else if(ch2 == 'A' || ch2 == 'B' || ch2 == 'C' || ch2 == 'D') return ch2;
    }
    else if(ch1 == 'h' || ch1 == 'q' || ch1 == ':' || ch1 == 127 || ch1 == 13) return ch1;
    return -1;
}

// Key Presses in Normal Mode.
void handleKeyPressesInNormalMode() {
    while(true) {
        char finalChar = '\0';
        char ch;
        read(STDIN_FILENO, &ch, 1);
        if(ch == 'h' || ch == 'q' || ch == ':' || ch == 127 || ch == 13) {
            finalChar = ch;
        }
        // Escape Character
        else if (ch == 27) {
            char tempChar = readEscape();
            if(tempChar != -1) finalChar = tempChar;
        }

        // Quit - Close the Application
        if(finalChar == 'q') { 
            clearScreen();
            // cout << "Thanks for using File Explorer.\r\n";
            exit(1);
        }
        
        // : - Enter the Command Mode 
        else if(finalChar == ':') {
            normalMode = false;
            handleKeyPressesInCommandMode();
            printDirInfo(currentWorkingDirectory);
        }
        
        // h - Open Home Directory.
        else if(finalChar == 'h') {
            rowIndex = 0;
            windowStartIndex = 0;
            clearForwardStack();
            if(currentWorkingDirectory != homeDirectory) {
                backwardStack.push(currentWorkingDirectory);
                currentWorkingDirectory = homeDirectory;
            }
            
            printDirInfo(currentWorkingDirectory);
            // cout << "Home\r\n";
        }
        
        // Backspace - Going to Parent Directory
        else if(finalChar == 127) {
            string newDirectory = getParentDirectory(currentWorkingDirectory);
            if(newDirectory != currentWorkingDirectory) {
                clearForwardStack();
                backwardStack.push(currentWorkingDirectory);
            }
            rowIndex = 0;
            windowStartIndex = 0;
            currentWorkingDirectory = newDirectory;
            printDirInfo(currentWorkingDirectory);
            // cout << "BackSpace\r\n";
        }
        
        // Enter - Open the Directory/File
        else if(finalChar == 13) {
            // cout << fileName;
            if(dirInfo[rowIndex][0] == ".") {}
            else if (dirInfo[rowIndex][0] == "..") {
                string newDirectory = getParentDirectory(currentWorkingDirectory);
                if(newDirectory != currentWorkingDirectory) {
                    clearForwardStack();
                    backwardStack.push(currentWorkingDirectory);
                }
                rowIndex = 0;
                windowStartIndex = 0;
                currentWorkingDirectory = newDirectory;
                printDirInfo(currentWorkingDirectory);
            }
            else if(isDirectory(dirInfo[rowIndex][1] + dirInfo[rowIndex][0])) {
                clearForwardStack();
                backwardStack.push(currentWorkingDirectory);
                string newDirectory = dirInfo[rowIndex][1] + dirInfo[rowIndex][0] + "/";
                rowIndex = 0;
                windowStartIndex = 0;
                currentWorkingDirectory = newDirectory;
                printDirInfo(currentWorkingDirectory);
            }
            else {
                openFile(dirInfo[rowIndex][1] + dirInfo[rowIndex][0]);
            }
            // cout << "Enter\r\n";
        }
        
        // Up Key - Scroll Up
        else if(finalChar == 'A') {
            if(rowIndex > 0) rowIndex--;
            if(rowIndex < windowStartIndex) windowStartIndex--;
            printDirInfo(currentWorkingDirectory);
            // cout << "Up\r\n";
        }
        
        // Down Key - Scroll Down
        else if(finalChar == 'B') {
            if(rowIndex < dirInfo.size() - 1) rowIndex++;
            if(rowIndex >= maxRowsInNormalMode) windowStartIndex++;
            printDirInfo(currentWorkingDirectory);
        }

        // Right Key - For Next Directory
        else if(finalChar == 'C') {
            if(!forwardStack.empty()) {
                backwardStack.push(currentWorkingDirectory);
                currentWorkingDirectory = forwardStack.top();
                forwardStack.pop();
                printDirInfo(currentWorkingDirectory);
            }
        }

        // Left Key - For Previous Visited Directory
        else if(finalChar == 'D') {
            if(!backwardStack.empty()) {
                forwardStack.push(currentWorkingDirectory);
                currentWorkingDirectory = backwardStack.top();
                backwardStack.pop();
                printDirInfo(currentWorkingDirectory);
            }
        }

    }
}

void resizeSignalHandler(int signal_num) {
    // cout << "The interrupt signal is (" << signal_num << "). \n";
    if (SIGWINCH == signal_num) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if(w.ws_row < windowRows && rowIndex == maxRowsInNormalMode-1) windowStartIndex++;
        if(w.ws_row > windowRows && windowStartIndex > 0) windowStartIndex--;
        windowRows = w.ws_row;
        windowCols = w.ws_col;
        printDirInfo(currentWorkingDirectory);

        // cout << "Interrupt Generated : ";
        // printf ("lines %d --", w.ws_row);
        // printf ("columns %d\r\n", w.ws_col);
    // printf("SIGWINCH raised, window size: %d rows / %d columns\n",
    }
}

void testCode() {
    // getAbsolutePath("/home/mayank/../mayank/./.././");
}


int main(int argc, char **argv)
{
    // testCode();
    // while(1);
    // Signal Handler - For Resizing the Window in Real Time. 
    // When someone resizes the window, resizeSignalHandler will be called.
    signal(SIGWINCH, resizeSignalHandler);
    
    // Calculating Initial Window Size, when the Application is launched for first time.
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    windowRows = w.ws_row;
    windowCols = w.ws_col;

    // Fetching the home directory of current user of the system.
    getHomeDirectory();
    
    // Fetching the current directory where the application is launched, and sed setting it as current working directory.
    // So, the Default page when the Application will start, will be current working Directory.
    currentWorkingDirectory = get_current_dir_name();
    if(currentWorkingDirectory[currentWorkingDirectory.size() - 1] != '/') currentWorkingDirectory += '/';

    // Entering Raw Mode.(It will handle key presses in Real Time.)
    enableRawMode();
    
    printDirInfo(currentWorkingDirectory);
    handleKeyPressesInNormalMode();


    // printDirInfo(currentWorkingDirectory);
    // cout << isDirectory("/") << endl;
    // printDirInfo("/home/mayank/Desktop/IIITH Courses/AOS/Assignment/Assignment1/2022201012/");
    // cout << homeDirectory << endl;
    // cout << "\033[7m" << "Mayank Gupta" << "\n";
    // printDirInfo("./");
    
    
    // getDirectoryInfo("./");
    
    return 0;
}