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

using namespace std;

vector<vector<string>> dirInfo; // For Storing directory info (including sub-directory & Files)

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
    
    cout << fileSize <<"\t";

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
        cout << "Can't Open File Directory\n";
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
  
void printDirInfo(string path) {
    getDirectoryInfo(path);
    for(int dirIndex = 0 ; dirIndex < dirInfo.size() ; dirIndex++) {
        vector<string> fileInfo = dirInfo[dirIndex];
        cout << fileInfo[1] << "\t" << fileInfo[2] << "\t" << fileInfo[3] << "\t" << fileInfo[4] << "\t" << fileInfo[5] << "\t" << fileInfo[6] << "\t" << fileInfo[0] << "\n";
    }
}


void testCode() {

}

int main(int argc, char **argv)
{
    // testCode();
    printDirInfo("/");
        
    // getDirectoryInfo("./");
    
    return 0;
}