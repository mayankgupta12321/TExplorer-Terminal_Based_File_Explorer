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

#define maxRowsInNormalMode (windowStartIndex + windowRows - 9)
#define cursorSelected "⮕ "
#define cursorUnselected "  "

using namespace std;


/******************************************
    Variable Declaration                  *
*******************************************/

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

bool success = false;
bool failure = false;

string success_message ;
string failure_message ;

/******************************************
    Function Declaration                  *
*******************************************/
void initialise();
void showCursor();
void hideCursor();
void enableRawMode();
void disableRawMode();
void resizeSignalHandler(int signal_num);
void clearScreen();
void getHomeDirectory();
void printDirInfo(string path);
void getDirectoryInfo(string path);
vector<string> getFileInfo(string fileName, string filePath);
long long int calculateSize(string dirPath);
string resizeFileInfo(vector<string> fileInfo);
string GetTimeAndDate(unsigned long long sec);
string convertSize(long long int fileSize);
bool isDirectory(string path);
bool isFile(string path);
string getAbsolutePath(string path);
string removeRedundancyFromPath(string path);

void handleKeyPressesInNormalMode();
int readEscape();
void clearForwardStack();
string getParentDirectory(string path);
void openFile(string filePath);


void handleKeyPressesInCommandMode();
void processBufferStringAndDoDesiredOperation();
void goToPath(string path) ;
bool search(string filename, string path);
bool createFile(string dirname, string pathname);
bool createDirectory(string dirname, string pathname);
bool copyDirectory(string directorySourcePath, string directoryDestinationPath);
bool copyFile(string fileSourcePath, string fileDestinationPath);
bool removeFile(string pathname);
bool removeDirectory(string pathname);
bool deleteDirectory(string directoryPath);
bool renameFileOrDirectory(string path1, string path2);

// ------------------------------------------------------------------

/******************************************
    Driver Function                       *
*******************************************/
int main(int argc, char **argv)
{
    initialise();
    
    return 0;
}

// Initialisation the Application.
void initialise() {

    // Signal Handler - For Resizing the Window in Real Time. 
    // When someone resizes the window, resizeSignalHandler will be called.
    signal(SIGWINCH, resizeSignalHandler);

    hideCursor(); //As Default mode is normal Mode, so hiding the cursor.

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
    getDirectoryInfo(currentWorkingDirectory);
    printDirInfo(currentWorkingDirectory);
    handleKeyPressesInNormalMode();

}

// Display the cursor
void showCursor() {
    cout << "\033[?25h"; // Show Cursor
}

// Hide the cursor
void hideCursor() {
    cout << "\033[?25l"; // Hide Cursor
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
    if(tcgetattr(STDIN_FILENO, &original_termios) == -1) 
    {
        cout << "Error\n";
        cout.flush();
    }
    atexit(disableRawMode);

    struct termios raw_termios = original_termios;
    raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cflag |= (CS8);
    raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1) {
        cout << "Error\n";
        cout.flush();
    }
}

// Enables Raw Mode (Canonical Mode)
void disableRawMode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) {
        cout << "Error\n";
        cout.flush();
    }
}

// Handles Window Resizing in Real Time.
void resizeSignalHandler(int signal_num) {
    if (SIGWINCH == signal_num) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if(w.ws_row < windowRows && rowIndex == maxRowsInNormalMode-1) windowStartIndex++;
        if(w.ws_row > windowRows && windowStartIndex > 0) windowStartIndex--;
        windowRows = w.ws_row;
        windowCols = w.ws_col;
        printDirInfo(currentWorkingDirectory);

    }
}

// Clears the Screen.
void clearScreen() {
    cout << "\033[H\033[2J\033[3J"; // ANSI escape Sequence to clear screen.
}

// Fetching Current System User, and storing it in global variable systemUserName
void getHomeDirectory() {
    homeDirectory = "/home/" + (string)getpwuid(getuid())->pw_name + "/";
}

// Print Directory Info
void printDirInfo(string path) {
    clearScreen(); //Clearing the Screen before Printing.
    // getDirectoryInfo(path);
    string header = " File Name                      Size    User Name      Group Name     Permission   Last Modified Date";
    header = header.substr(0,windowCols-5);
    cout << "\033[1;4;33m" << header << "\033[0m" << "\r\n";
    for(int dirIndex = windowStartIndex ; dirIndex < maxRowsInNormalMode; dirIndex++) {
        if(dirIndex >= dirInfo.size()) {
            cout << "\r\n";
            continue;
        }
        vector<string> fileInfo = dirInfo[dirIndex];
        if(normalMode == true && dirIndex == rowIndex) {           
            cout<<"\033[1;7m";
            cout << cursorSelected;
            cout << resizeFileInfo(fileInfo) << "\r\n";
            cout<<"\033[0m";
        }
        else {
            cout << cursorUnselected << resizeFileInfo(fileInfo) << "\r\n";
        }
    }
    cout << "\033[1;34m" << "------------------|" << "\033[0m" << "\r\n";
    if(normalMode) {
        cout << "\033[1;33m" << " > Normal Mode\t  " << "\033[0m";
    }
    else {
        cout << "\033[1;35m" << " > Command Mode\t  " << "\033[0m";
    }
    cout << "\033[1;34m" << "|" << "\033[0m";

    if(failure) {
        cout << " ";
        cout << "\033[1;31m" << " :-( " << failure_message << "\033[0m";
        cout.flush();
        failure_message = "";
        failure = false;        
        success_message = "";
        success = false;
    }
    else if(success) {
        cout << " ";
        cout << "\033[1;32m" << " :-) " << success_message << "\033[0m";
        cout.flush();
        success_message = "";
        success = false;
    }
    

    cout << "\r\n";
    cout << "\033[1;34m" << "------------------|" << "\033[0m" << "\r\n";
    
    cout<<"\033[1;36m";
    cout << currentWorkingDirectory;
    cout<<"\033[0m";
    // cout.flush();
    if(normalMode == 0) {
        cout << "$ " << inputCommandString ;
    }
    cout.flush();
    if(normalMode) {
        cout << "\r\n\r\n\r\n";
        cout << " Created By :- Mayank Gupta - 2022201012 - IIIT Hyderabad.";
        cout.flush();
    }
}

// Stores the directory/Files info into global vector dirInfo.
void getDirectoryInfo(string path) {
    struct dirent *de;
    DIR *dr = opendir(path.c_str());
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        currentWorkingDirectory = backwardStack.top();
        backwardStack.pop();

        int timer = 5;

        while(timer) {
            clearScreen();
            cout << "\r\n";
            cout << "\033[91m" << " Either the directory you want to visit doesn't Exists, or you don't have read-write permissions..\r\n" << "\033[0m";
            cout << "\033[33m" << " Redirecting you to " << "\033[0m";
            cout << "\033[93m" << currentWorkingDirectory << "\033[0m";
            cout << "\033[33m" <<" in " << "\033[0m";
            cout << "\033[1;4;31m" << timer << "\033[0m";
            cout << "\033[33m"  << " seconds. \r\n" << "\033[0m";;
            cout << "\033[1;94m"  << " Please wait...\r\n" << "\033[0m";
            sleep(1);
            timer--;
        }
        
        return;
    }
    
    dirInfo.clear();
    vector<string> fileNames; // For storing names of files & sub-directories
    
    while ((de = readdir(dr)) != NULL){
        fileNames.push_back(de->d_name);
    }
    closedir(dr);

    // Sorting the file names
    sort(fileNames.begin() , fileNames.end());

    // Storing the metadata of files/directories of current working directory in a vector.
    for(string fileName : fileNames) {
        vector<string> fileInfo = getFileInfo(fileName, path);
        dirInfo.push_back(fileInfo);
    }
  
}   

// for fetching File Info stats like git status(fileSize , username, groupname, last modified date, file permission)
vector<string> getFileInfo(string fileName, string filePath) {
    vector<string> fileInfo;

    string fullFileName = filePath + fileName;

    struct stat fileStat;
    if(stat(fullFileName.c_str(),&fileStat) < 0)   {
        failure = 1;
        failure_message = "File Not Exists.";
        return {};
    }
    
    string fileSize; 
    if(S_ISDIR(fileStat.st_mode)) {
        fileSize = convertSize(calculateSize(getAbsolutePath(fullFileName)/*, getAbsolutePath(filePath)*/ ));
    }
    else {
        fileSize = convertSize(fileStat.st_size);
    }
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

// Checks if the given file/directory is present in current working directory or not. (Need to check Recursively)
long long int calculateSize(string dirPath) {
    long long int cur_size = 0;
    struct dirent *de;
    DIR *dr = opendir(dirPath.c_str());
    if (dr != NULL) {
        while ((de = readdir(dr)) != NULL) {
            string d_name = de->d_name;
            if(d_name == "." || d_name == "..") {
                continue;
            }
            
            string newPath = dirPath + d_name + "/";
            string fullFileName = dirPath + d_name;
            if(isDirectory(newPath)) {
                    cur_size += calculateSize(newPath);
            }
            else {
                struct stat fileStat;
                if(stat(fullFileName.c_str(),&fileStat) >= 0) {
                    cur_size += fileStat.st_size;
                }
            }
        }
    }
    closedir(dr);
    return cur_size;
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
    while(userName.size() < 12) {
        userName += ' ';
    }
    if(userName.size() > 12) {
        userName = userName.substr(0, 7) + "..."; 
    }

    // GroupName
    string groupName = fileInfo[4];
    while(groupName.size() < 12) {
        groupName += ' ';
    }
    if(groupName.size() > 12) {
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

// Convert Seconds into date/time;
string GetTimeAndDate(unsigned long long sec) {
    char date[100];
    time_t seconds = (time_t)(sec);
    strftime(date, sizeof(date) - 1, "%d-%b-%Y %R ", localtime(&seconds));
    struct tm *tmmm = localtime(&seconds);
    return date;
}

// Convert fileSize from Bytes to B/KB/MB/GB/TB
string convertSize(long long int fileSize) {
    long long int kb = 1024;    //KB
    long long int mb = 1048576; //MB
    long long int gb = 1073741824; //GB

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

// Check If given path is a directory
bool isFile(string path) {
    struct stat fileStat;
    stat(path.c_str(),&fileStat);
    return S_ISREG(fileStat.st_mode);
}

// returns the absolute path, irrespetive of when the inputh path is relative or absolute.
string  getAbsolutePath(string path) {
    string absolutePath = "";
    if(path[0] == '/') {
        absolutePath = path;
    }
    else if(path[0] == '~') {
        if(path.size() <= 2) absolutePath = homeDirectory;
        else absolutePath = homeDirectory + path.substr(2);
    }
    else {
        absolutePath = currentWorkingDirectory + path;
    }

    if(absolutePath[absolutePath.size() - 1] != '/') {
        absolutePath += '/';
    }
    absolutePath = removeRedundancyFromPath(absolutePath);
    return absolutePath;
}

// Remove extra '.' and '..' from the path.
string removeRedundancyFromPath(string path) {
    vector<string> strArr;
    stringstream ss (path);
    string item;
    while (getline (ss, item, '/')) {
        if(item == "" || item == ".") continue;
        if(item == "..") {
            if(strArr.size() != 0) strArr.pop_back();
            continue;
        }
        strArr.push_back (item);
    }
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

// Key Presses in Normal Mode.
void handleKeyPressesInNormalMode() {   
    while(true) {
        char finalChar = '\0';
        char ch;
        read(STDIN_FILENO, &ch, 1);

        if(ch == 'h' || ch == 'q' || ch == ':' || ch == 127 || ch == 13) {
            finalChar = ch;
        }

        // Escape('Esc') Character
        else if (ch == 27) {
            char tempChar = readEscape();
            if(tempChar != -1) finalChar = tempChar;
        }

        // Quit('q') - Close the Application
        if(finalChar == 'q') { 
            clearScreen();
            cout << "\r\n";
            cout << "\033[1;35m" << " Thanks for using File Explorer." << "\033[0m";
            cout << "\r\n\r\n";
            exit(1);
        }
        
        // : - Enter the Command Mode 
        else if(finalChar == ':') {
            normalMode = false;
            showCursor();
            handleKeyPressesInCommandMode();
            hideCursor();
            getDirectoryInfo(currentWorkingDirectory);
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
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
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
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
        }
        
        // Enter - Open the Directory/File
        else if(finalChar == 13) {
            // If open '.' - means current directory, then do nothing.
            if(dirInfo[rowIndex][0] == ".") {}
            
            // If open '..' - means parent directory, then opening parent directory.
            else if (dirInfo[rowIndex][0] == "..") {
                string newDirectory = getParentDirectory(currentWorkingDirectory);
                if(newDirectory != currentWorkingDirectory) {
                    clearForwardStack();
                    backwardStack.push(currentWorkingDirectory);
                }
                rowIndex = 0;
                windowStartIndex = 0;
                currentWorkingDirectory = newDirectory;
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
            }
            
            // If the clicked path is Directory, then open that directory.
            else if(isDirectory(dirInfo[rowIndex][1] + dirInfo[rowIndex][0])) {
                clearForwardStack();
                backwardStack.push(currentWorkingDirectory);
                string newDirectory = dirInfo[rowIndex][1] + dirInfo[rowIndex][0] + "/";
                rowIndex = 0;
                windowStartIndex = 0;
                currentWorkingDirectory = newDirectory;
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
            }
            
            // If the clicked path is a File, then open that file.
            else {
                openFile(dirInfo[rowIndex][1] + dirInfo[rowIndex][0]);
            }
        }
        
        // Up Key - Scroll Up
        else if(finalChar == 'A') {
            if(rowIndex > 0) rowIndex--;
            if(rowIndex < windowStartIndex) windowStartIndex--;
            printDirInfo(currentWorkingDirectory);
        }
        
        // Down Key - Scroll Down
        else if(finalChar == 'B') {
            if(rowIndex < dirInfo.size() - 1) rowIndex++;
            if(rowIndex >= maxRowsInNormalMode) windowStartIndex++;
            printDirInfo(currentWorkingDirectory);
        }

        // Right Key - For Next Directory
        else if(finalChar == 'C') {
            windowStartIndex = 0;
            rowIndex = 0; 
            if(!forwardStack.empty()) {
                backwardStack.push(currentWorkingDirectory);
                currentWorkingDirectory = forwardStack.top();
                forwardStack.pop();
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
            }
        }

        // Left Key - For Previous Visited Directory
        else if(finalChar == 'D') {
            if(!backwardStack.empty()) {
                windowStartIndex = 0;
                rowIndex = 0; 
                forwardStack.push(currentWorkingDirectory);
                currentWorkingDirectory = backwardStack.top();
                backwardStack.pop();
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
                
            }
        }

    }
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

// Clears Forward Stack
void clearForwardStack() {
    while (!forwardStack.empty()) {
        forwardStack.pop();
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
}

// Opens the file in default editor
void openFile(string filePath) {
    const char *fileName = filePath.c_str();
    pid_t pid = fork();
    if(pid == 0) {
        execlp("xdg-open", "xdg-open", fileName, (char *)0);
    }
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
        failure = 1;
        failure_message = "Please provide any Input.";
        printDirInfo(currentWorkingDirectory);
    }
    
    // Quit
    else if(v[0] == "quit") {
        clearScreen();
        cout << "\r\n";
        cout << "\033[1;35m" << " Thanks for using File Explorer." << "\033[0m";
        cout << "\r\n\r\n";
        exit(1);
    }

    // Goto
    else if(v[0] == "goto") {
        if(v.size() == 2) {
            goToPath(v[1]);
        }
        else {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
    } 
    
    // Search
    else if(v[0] == "search") {
        if(currentWorkingDirectory == "/") {
            failure = 1;
            failure_message = "You don't have permission to do operation on root directory.";
            printDirInfo(currentWorkingDirectory);
        }
        else if(v.size() == 2) {
            if(search(v[1], currentWorkingDirectory)) {
                success = 1;
                success_message = "True. File Found.";
                printDirInfo(currentWorkingDirectory);
            } else {
                failure = 1;
                failure_message = "False. File Not Found.";
                printDirInfo(currentWorkingDirectory);
            }
        }
        else {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
    } 

    // Create File
    else if(v[0] == "create_file") {
        if(v.size() == 3) {
            if(createFile(v[1] , getAbsolutePath(v[2]))){
                success = 1;
                success_message = "File Created.";
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
            }
            else {
                failure = 1;
                failure_message = "File Not Created.";
                printDirInfo(currentWorkingDirectory);
            }
        }
        else {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
    } 

    // Create Directory
    else if(v[0] == "create_dir") {
        if(v.size() == 3) {
            if(createDirectory(v[1] , getAbsolutePath(v[2]))) {
                success = 1;
                success_message = "Directory Created.";
                getDirectoryInfo(currentWorkingDirectory);
                printDirInfo(currentWorkingDirectory);
            }
            else {
                failure = 1;
                failure_message = "Directory Not Created.";
                printDirInfo(currentWorkingDirectory);
            }
        }
        else {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
    } 

    // Copy
    else if(v[0] == "copy") {
        if(v.size() <= 2) {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
        else {
            string destinationPath = getAbsolutePath(v[v.size()-1]);
            int cnt_copied = 0;
            for(int i = 1 ; i < v.size() - 1 ; i++) {
                string sourcePath = getAbsolutePath(v[i]);
                if(isDirectory(sourcePath)) {
                    if(copyDirectory(sourcePath, destinationPath)){
                        cnt_copied++;
                    }
                }
                else {                
                    if(sourcePath[sourcePath.size() - 1] == '/') sourcePath = sourcePath.substr(0, sourcePath.size()-1);     
                    if(isFile(sourcePath)) {
                        int found = sourcePath.find_last_of('/');
                        string filePath = sourcePath.substr(0, found + 1);
                        string fileName = sourcePath.substr(found + 1 , sourcePath.size() - found + 1);
                        if(copyFile(sourcePath, destinationPath + fileName)){
                            cnt_copied++;
                        }
                    }
                    
                }
            }

            if(cnt_copied) {
                success = 1;
                success_message = to_string(cnt_copied) + " Files/Directories Copied.";
            }
            else {
                failure = 1;
                failure_message = "No Files/Directories Copied.";
            }
            printDirInfo(currentWorkingDirectory);
        }
    }

    // Delete File
    else if(v[0] == "delete_file") {
        if(v.size() < 2) {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
        else {
            int cnt_deleted = 0;
            for(int i = 1 ; i < v.size() ; i++) {
                if(removeFile(getAbsolutePath(v[i]))) {
                    cnt_deleted++;
                }
            }

            if(cnt_deleted) {
                success = 1;
                success_message = to_string(cnt_deleted) + " Files Deleted.";
            }
            else {
                failure = 1;
                failure_message = "No Files Deleted.";
            }
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
        }
    } 

    // Delete Directory
    else if(v[0] == "delete_dir") {
        if(v.size() < 2) {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
        else {
            int cnt_deleted = 0;
            for(int i = 1 ; i < v.size() ; i++) {
                if(deleteDirectory(getAbsolutePath(v[i]))) {
                    cnt_deleted++;
                }
            }
            if(cnt_deleted) {
                success = 1;
                success_message = "Delete Operation Successful.";
            }
            else {
                failure = 1;
                failure_message = "Delete Operation Failed.";
            }
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
        }
    } 

    // Rename
    else if(v[0] == "rename") {
        if(v.size() == 3) {
            if(renameFileOrDirectory(getAbsolutePath(v[1]), getAbsolutePath(v[2]))) {
                success = 1;
                success_message = "File/Directory Renamed Successfully.";
            }
            else {
                failure = 1;
                failure_message = "Rename Operation Failed.";
            }
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
        }
        else {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
    } 
    
    // Move
    else if(v[0] == "move") {
        if(v.size() <= 2) {
            failure = 1;
            failure_message = "Invalid number of arguments.";
            printDirInfo(currentWorkingDirectory);
        }
        else {
            string destinationPath = getAbsolutePath(v[v.size()-1]);
            int cnt_moved = 0;
            for(int i = 1 ; i < v.size() - 1 ; i++) {
                string sourcePath = getAbsolutePath(v[i]);
                if(isDirectory(sourcePath)) {
                    if(copyDirectory(sourcePath, destinationPath)){
                        if(deleteDirectory(getAbsolutePath(v[i]))) {
                            cnt_moved++;
                        }
                    }
                }
                else {                
                    if(sourcePath[sourcePath.size() - 1] == '/') sourcePath = sourcePath.substr(0, sourcePath.size()-1);
                    
                    if(isFile(sourcePath)) {
                        int found = sourcePath.find_last_of('/');
                        string filePath = sourcePath.substr(0, found + 1);
                        string fileName = sourcePath.substr(found + 1 , sourcePath.size() - found + 1);
                        if(copyFile(sourcePath, destinationPath + fileName)){
                            if(removeFile(getAbsolutePath(v[i]))) {
                                cnt_moved++;
                            }
                        }
                    }
                    
                }
            }

            if(cnt_moved) {
                success = 1;
                success_message = to_string(cnt_moved) + " Files/Directories Moved.";
            }
            else {
                failure = 1;
                failure_message = "No Files/Directories Moved.";
            }
            getDirectoryInfo(currentWorkingDirectory);
            printDirInfo(currentWorkingDirectory);
        }
    }

    // Undefined Command
    else {
        failure = 1;
        failure_message = "Invalid Input!!!";
        printDirInfo(currentWorkingDirectory);
    }
}

// Switch to any path.
void goToPath(string path) {
    string newPath = getAbsolutePath(path);
    if(newPath == currentWorkingDirectory) {
        printDirInfo(currentWorkingDirectory);
        return;
    }
    clearForwardStack();
    backwardStack.push(currentWorkingDirectory);
    currentWorkingDirectory = newPath;
    inputCommandString = "";
    getDirectoryInfo(currentWorkingDirectory);
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
            if(d_name == filename) {
                return true;
            }
            string newPath = path + d_name + "/";
            if(isDirectory(newPath)) {
                if(search(filename, newPath)) {
                    return true;
                }
            }
        }
    }
    closedir(dr);
    return 0;
}

// Create File
bool createFile(string dirname, string pathname) {
    string _file = pathname + dirname;
    if(creat(_file.c_str() , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) < 0) return false;
    return true;
}

// Create Directory
bool createDirectory(string dirname, string pathname) {
    string _dir = pathname + dirname;
    if(mkdir(_dir.c_str() , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) return false;
    return true;
}

// Create Directory in Destination Path recursively with same Permission as Source.
bool copyDirectory(string directorySourcePath, string directoryDestinationPath) {

    if(directorySourcePath == "/") return false;
    if(directorySourcePath[directorySourcePath.size() - 1] == '/') directorySourcePath = directorySourcePath.substr(0, directorySourcePath.size()-1);
    int found = directorySourcePath.find_last_of('/');
    
    string dirPath = directorySourcePath.substr(0, found + 1);
    string dirName = directorySourcePath.substr(found + 1 , directorySourcePath.size() - found + 1);

    if(dirPath == directoryDestinationPath) return true;

    createDirectory(dirName, directoryDestinationPath);

    // Fetching Source Directory ownership & permission details.
    struct stat fileStat;
    stat(directorySourcePath.c_str(),&fileStat);

    string directoryDestinationNewPath = directoryDestinationPath + dirName + '/';

    // Setting Destination Directory ownership & permission details.
    if(chmod(directoryDestinationNewPath.c_str(), fileStat.st_mode) < 0) return false;
    if(chown(directoryDestinationNewPath.c_str(), fileStat.st_uid, fileStat.st_gid) < 0) return false;

    struct dirent *de;
    DIR *dr = opendir(directorySourcePath.c_str());
    if (dr != NULL) {
        while ((de = readdir(dr)) != NULL) {
            string d_name = de->d_name;
            if(d_name == "." || d_name == "..") {
                continue;
            }
            string newSourcePath = directorySourcePath + '/' + d_name;
            if(isDirectory(newSourcePath)) {
                
                if(copyDirectory(newSourcePath, directoryDestinationNewPath) < 0) return false;
            }
            else {
                copyFile(newSourcePath, directoryDestinationNewPath + d_name);
            }
        }
    }
    closedir(dr);


    return true;   
}

// Copy File
bool copyFile(string fileSourcePath, string fileDestinationPath) {
    if(fileSourcePath == fileDestinationPath) return true;
    int nread;
    char buffer[8092];
    int fd_open = open(fileSourcePath.c_str(), O_RDONLY);
    int fd_close = open(fileDestinationPath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd_open < 0 || fd_close < 0) return false;

    while((nread = read(fd_open,buffer,sizeof(buffer))) > 0) {
        write(fd_close,buffer,nread);
    }

    // Fetching Source File ownership & permission details.
    struct stat fileStat;
    stat(fileSourcePath.c_str(),&fileStat);

    // Setting Destination File ownership & permission details.
    if(chmod(fileDestinationPath.c_str(), fileStat.st_mode) < 0) return false;
    if(chown(fileDestinationPath.c_str(), fileStat.st_uid, fileStat.st_gid) < 0) return false;;
    
    close(fd_open);
    close(fd_close);
    return true;
}

// Remove File
bool removeFile(string pathname) {
    if(pathname[pathname.size() - 1] == '/') pathname = pathname.substr(0, pathname.size()-1);
    if(remove(pathname.c_str()) < 0) return false;
    return true;
}

// Remove Directory
bool removeDirectory(string pathname) {
    if(rmdir(pathname.c_str()) < 0) return false;
    return true;
}

// Delete Directory recursively.
bool deleteDirectory(string directoryPath) {

    if(directoryPath == "/") return false;

    struct stat fileStat;
    stat(directoryPath.c_str(),&fileStat);

    struct dirent *de;
    DIR *dr = opendir(directoryPath.c_str());

    if (dr != NULL) {
        // First Deleting all the files & folder within current directory recursively.
        while ((de = readdir(dr)) != NULL) {
            string d_name = de->d_name;
            if(d_name == "." || d_name == "..") {
                continue;
            }
            if(isDirectory(directoryPath + d_name)) {
                if(deleteDirectory(directoryPath + d_name + '/') < 0) return false;
            }
            else {
                if(removeFile(directoryPath + d_name) < 0) return false;
            }
        }
    }
    closedir(dr);

    // Then Deleting the directory itself.
    if(removeDirectory(directoryPath) < 0) return false;

    return true;   
}

// Rename File
bool renameFileOrDirectory(string path1, string path2) {
    if(path1[path1.size() - 1] == '/') path1 = path1.substr(0, path1.size()-1);
    if(path2[path2.size() - 1] == '/') path2 = path2.substr(0, path2.size()-1);
    if(path1 == path2) return true;
    if(rename(path1.c_str(), path2.c_str()) < 0) return false;
    return true;
}
