#include "ofFileUtils.h"
#ifndef TARGET_WIN32
	#include <pwd.h>
	#include <sys/stat.h>
#endif

#include "ofUtils.h"


#ifdef TARGET_OSX
	#include <mach-o/dyld.h>       /* _NSGetExecutablePath */
	#include <limits.h>        /* PATH_MAX */
#endif


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- ofBuffer
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


size_t ofBuffer::ioSize = 1024;

//--------------------------------------------------
ofBuffer::ofBuffer()
:currentLine(end(),end()){
	buffer.resize(1);
}

//--------------------------------------------------
ofBuffer::ofBuffer(const char * _buffer, unsigned int size)
:buffer(_buffer,_buffer+size)
,currentLine(end(),end()){
	buffer.resize(buffer.size()+1,0);
}

//--------------------------------------------------
ofBuffer::ofBuffer(const string & text)
:buffer(text.begin(),text.end())
,currentLine(end(),end()){
	buffer.resize(buffer.size()+1,0);
}

//--------------------------------------------------
ofBuffer::ofBuffer(istream & stream)
:currentLine(end(),end()){
	set(stream);
}

//--------------------------------------------------
bool ofBuffer::set(istream & stream){
	if(stream.bad()){
		clear();
		return false;
	}else{
		buffer.clear();
	}

	vector<char> aux_buffer(ioSize);
	while(stream.good()){
		stream.read(&aux_buffer[0], ioSize);
		buffer.insert(buffer.end(),aux_buffer.begin(),aux_buffer.begin()+stream.gcount());
	}
	buffer.push_back(0);
	return true;
}

//--------------------------------------------------
bool ofBuffer::writeTo(ostream & stream) const {
	if(stream.bad()){
		return false;
	}
	stream.write(&(buffer[0]), buffer.size() - 1);
	return true;
}

//--------------------------------------------------
void ofBuffer::set(const char * _buffer, unsigned int _size){
	buffer.assign(_buffer,_buffer+_size);
	buffer.resize(buffer.size()+1,0);
}

//--------------------------------------------------
void ofBuffer::set(const string & text){
	set(text.c_str(),text.size());
}

//--------------------------------------------------
void ofBuffer::append(const string& _buffer){
	append(_buffer.c_str(), _buffer.size());
}

//--------------------------------------------------
void ofBuffer::append(const char * _buffer, unsigned int _size){
	buffer.insert(buffer.end()-1,_buffer,_buffer+_size);
	buffer.back() = 0;
}

//--------------------------------------------------
void ofBuffer::clear(){
	buffer.resize(1,0);
}

//--------------------------------------------------
void ofBuffer::allocate(long _size){
	clear();
	//we always add a 0 at the end to avoid problems with strings
	buffer.resize(_size + 1, 0);
}

//--------------------------------------------------
char * ofBuffer::getData(){
	if(buffer.empty()){
		return NULL;
	}
	return &buffer[0];
}

//--------------------------------------------------
const char * ofBuffer::getData() const{
	if(buffer.empty()){
		return NULL;
	}
	return &buffer[0];
}

//--------------------------------------------------
char *ofBuffer::getBinaryBuffer(){
	return getData();
}

//--------------------------------------------------
const char *ofBuffer::getBinaryBuffer() const {
	return getData();
}

//--------------------------------------------------
string ofBuffer::getText() const {
	if(buffer.empty()){
		return "";
	}
	return &buffer[0];
}

//--------------------------------------------------
ofBuffer::operator string() const {
	return getText();
}

//--------------------------------------------------
ofBuffer & ofBuffer::operator=(const string & text){
	set(text);
	return *this;
}

//--------------------------------------------------
long ofBuffer::size() const {
	if(buffer.empty()){
		return 0;
	}
	//we always add a 0 at the end to avoid problems with strings
	return buffer.size() - 1;
}

//--------------------------------------------------
void ofBuffer::setIOBufferSize(size_t _ioSize){
	ioSize = _ioSize;
}

//--------------------------------------------------
string ofBuffer::getNextLine(){
	if(currentLine.empty()){
		currentLine = getLines().begin();
	}else{
		++currentLine;
	}
	return currentLine.asString();
}

//--------------------------------------------------
string ofBuffer::getFirstLine(){
	currentLine = getLines().begin();
	return currentLine.asString();
}

//--------------------------------------------------
bool ofBuffer::isLastLine(){
	return currentLine == getLines().end();
}

//--------------------------------------------------
void ofBuffer::resetLineReader(){
	currentLine = getLines().begin();
}

//--------------------------------------------------
vector<char>::iterator ofBuffer::begin(){
	return buffer.begin();
}

//--------------------------------------------------
vector<char>::iterator ofBuffer::end(){
	return buffer.end();
}

//--------------------------------------------------
vector<char>::const_iterator ofBuffer::begin() const{
	return buffer.begin();
}

//--------------------------------------------------
vector<char>::const_iterator ofBuffer::end() const{
	return buffer.end();
}

//--------------------------------------------------
vector<char>::reverse_iterator ofBuffer::rbegin(){
	return buffer.rbegin();
}

//--------------------------------------------------
vector<char>::reverse_iterator ofBuffer::rend(){
	return buffer.rend();
}

//--------------------------------------------------
vector<char>::const_reverse_iterator ofBuffer::rbegin() const{
	return buffer.rbegin();
}

//--------------------------------------------------
vector<char>::const_reverse_iterator ofBuffer::rend() const{
	return buffer.rend();
}

//--------------------------------------------------
ofBuffer::Line::Line(vector<char>::iterator _begin, vector<char>::iterator _end)
	:_current(_begin)
	,_begin(_begin)
	,_end(_end){
	if(_begin == _end){
		line =  "";
		return;
	}

	bool lineEndWasCR = false;
	while(_current != _end && *_current != '\n'){
		if(*_current == '\r'){
			lineEndWasCR = true;
			break;
		}else if(*_current==0 && _current+1 == _end){
			break;
		}else{
			_current++;
		}
	}
	line = string(_begin, _current);
	if(_current != _end){
		_current++;
	}
	// if lineEndWasCR check for CRLF
	if(lineEndWasCR && _current != _end && *_current == '\n'){
		_current++;
	}
}

//--------------------------------------------------
const string & ofBuffer::Line::operator*() const{
	return line;
}

//--------------------------------------------------
const string * ofBuffer::Line::operator->() const{
	return &line;
}

//--------------------------------------------------
const string & ofBuffer::Line::asString() const{
	return line;
}

//--------------------------------------------------
ofBuffer::Line & ofBuffer::Line::operator++(){
	*this = Line(_current,_end);
	return *this;
}

//--------------------------------------------------
ofBuffer::Line ofBuffer::Line::operator++(int) {
	Line tmp(*this);
	operator++();
	return tmp;
}

//--------------------------------------------------
bool ofBuffer::Line::operator!=(Line const& rhs) const{
	return rhs._begin != _begin || rhs._end != _end;
}

//--------------------------------------------------
bool ofBuffer::Line::operator==(Line const& rhs) const{
	return rhs._begin == _begin && rhs._end == _end;
}

bool ofBuffer::Line::empty() const{
	return _begin == _end;
}

//--------------------------------------------------
ofBuffer::Lines::Lines(vector<char> & buffer)
:_begin(buffer.begin())
,_end(buffer.end()){}

//--------------------------------------------------
ofBuffer::Line ofBuffer::Lines::begin(){
	return Line(_begin,_end);
}

//--------------------------------------------------
ofBuffer::Line ofBuffer::Lines::end(){
	return Line(_end,_end);
}

//--------------------------------------------------
ofBuffer::Lines ofBuffer::getLines(){
	return ofBuffer::Lines(buffer);
}

//--------------------------------------------------
ostream & operator<<(ostream & ostr, const ofBuffer & buf){
	buf.writeTo(ostr);
	return ostr;
}

//--------------------------------------------------
istream & operator>>(istream & istr, ofBuffer & buf){
	buf.set(istr);
	return istr;
}

//--------------------------------------------------
ofBuffer ofBufferFromFile(const string & path, bool binary){
	ios_base::openmode mode = binary ? ifstream::binary : ios_base::in;
	ifstream istr(ofToDataPath(path, true).c_str(), mode);
	ofBuffer buffer(istr);
	istr.close();
	return buffer;
}

//--------------------------------------------------
bool ofBufferToFile(const string & path, ofBuffer & buffer, bool binary){
	ios_base::openmode mode = binary ? ofstream::binary : ios_base::out;
	ofstream ostr(ofToDataPath(path, true).c_str(), mode);
	bool ret = buffer.writeTo(ostr);
	ostr.close();
	return ret;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- ofFile
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
ofFile::ofFile()
:mode(Reference)
,binary(true){
}

ofFile::ofFile(const std::filesystem::path & path, Mode mode, bool binary)
:mode(mode)
,binary(true){
	open(path, mode, binary);
}

//-------------------------------------------------------------------------------------------------------------
ofFile::~ofFile(){
	//close();
}

//-------------------------------------------------------------------------------------------------------------
ofFile::ofFile(const ofFile & mom)
:mode(Reference)
,binary(true){
	copyFrom(mom);
}

//-------------------------------------------------------------------------------------------------------------
ofFile & ofFile::operator=(const ofFile & mom){
	copyFrom(mom);
	return *this;
}

//-------------------------------------------------------------------------------------------------------------
void ofFile::copyFrom(const ofFile & mom){
	if(&mom != this){
		Mode new_mode = mom.mode;
		if(new_mode != Reference && new_mode != ReadOnly){
			new_mode = ReadOnly;
			ofLogWarning("ofFile") << "copyFrom(): copying a writable file, opening new copy as read only";
		}
		open(mom.myFile.string(), new_mode, mom.binary);
	}
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::openStream(Mode _mode, bool _binary){
	mode = _mode;
	binary = _binary;
	ios_base::openmode binary_mode = binary ? ios::binary : (ios_base::openmode)0;
	switch(_mode) {
		case WriteOnly:
		case ReadWrite:
		case Append:
			if(!ofDirectory(ofFilePath::getEnclosingDirectory(path())).exists()){
				ofFilePath::createEnclosingDirectory(path());
			}
			break;
		case Reference:
		case ReadOnly:
			break;
	}
	switch(_mode){
		case Reference:
			return true;
			break;

		case ReadOnly:
			if(exists() && isFile()){
				fstream::open(path().c_str(), ios::in | binary_mode);
			}
			break;

		case WriteOnly:
			fstream::open(path().c_str(), ios::out | binary_mode);
			break;

		case ReadWrite:
			fstream::open(path().c_str(), ios_base::in | ios_base::out | binary_mode);
			break;

		case Append:
			fstream::open(path().c_str(), ios::out | ios::app | binary_mode);
			break;
	}
	return fstream::good();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::open(const std::filesystem::path & _path, Mode _mode, bool binary){
	close();
	myFile = std::filesystem::path(ofToDataPath(_path.string()));
	return openStream(_mode, binary);
}

//-------------------------------------------------------------------------------------------------------------
bool ofFile::changeMode(Mode _mode, bool binary){
	if(_mode != mode){
		string _path = path();
		close();
		myFile = std::filesystem::path(_path);
		return openStream(_mode, binary);
	}
	else{
		return true;
	}
}

//-------------------------------------------------------------------------------------------------------------
bool ofFile::isWriteMode(){
	return mode != ReadOnly;
}

//-------------------------------------------------------------------------------------------------------------
void ofFile::close(){
	myFile = std::filesystem::path();
	if(mode!=Reference) fstream::close();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::create(){
	bool success = false;

	if(!myFile.string().empty()){
		auto mode = this->mode;
		success = open(path(),ofFile::WriteOnly);
		open(path(),mode);
	}

	return success;
}

//------------------------------------------------------------------------------------------------------------
ofBuffer ofFile::readToBuffer(){
	if(myFile.string().empty() || !std::filesystem::exists(myFile)){
		return ofBuffer();
	}

	return ofBuffer(*this);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::writeFromBuffer(const ofBuffer & buffer){
	if(myFile.string().empty()){
		return false;
	}
	if(!isWriteMode()){
		ofLogError("ofFile") << "writeFromBuffer(): trying to write to read only file \"" << myFile.string() << "\"";
	}
	return buffer.writeTo(*this);
}

//------------------------------------------------------------------------------------------------------------
filebuf *ofFile::getFileBuffer() const {
	return rdbuf();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::exists() const {
	if(path().empty()){
		return false;
	}

	return std::filesystem::exists(myFile);
}

//------------------------------------------------------------------------------------------------------------
string ofFile::path() const {
	return myFile.string();
}

//------------------------------------------------------------------------------------------------------------
string ofFile::getExtension() const {
	auto dotext = myFile.extension().string();
	if(!dotext.empty() && dotext.front()=='.'){
		return std::string(dotext.begin()+1,dotext.end());
	}else{
		return dotext;
	}
}

//------------------------------------------------------------------------------------------------------------
string ofFile::getFileName() const {
	return myFile.filename().string();
}

//------------------------------------------------------------------------------------------------------------
string ofFile::getBaseName() const {
	return std::filesystem::basename(myFile);
}

//------------------------------------------------------------------------------------------------------------
string ofFile::getEnclosingDirectory() const {
	return ofFilePath::getEnclosingDirectory(path());
}

//------------------------------------------------------------------------------------------------------------
string ofFile::getAbsolutePath() const {
	return ofFilePath::getAbsolutePath(path());
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::canRead() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef TARGET_WIN32
	DWORD attr = GetFileAttributes(myFile.native().c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	return true;
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_read;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_read;
	}else{
		return perm & std::filesystem::others_read;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::canWrite() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef TARGET_WIN32
	DWORD attr = GetFileAttributes(myFile.native().c_str());
	if (attr == INVALID_FILE_ATTRIBUTES){
		return false;
	}else{
		return (attr & FILE_ATTRIBUTE_READONLY) == 0;
	}
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_write;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_write;
	}else{
		return perm & std::filesystem::others_write;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::canExecute() const {
	auto perm = std::filesystem::status(myFile).permissions();
#ifdef TARGET_WIN32
	return getExtension() == "exe";
#else
	struct stat info;
	stat(path().c_str(), &info);  // Error check omitted
	if(geteuid() == info.st_uid){
		return perm & std::filesystem::owner_exe;
	}else if (getegid() == info.st_gid){
		return perm & std::filesystem::group_exe;
	}else{
		return perm & std::filesystem::others_exe;
	}
#endif
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::isFile() const {
	return std::filesystem::is_regular_file(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::isLink() const {
	return std::filesystem::is_symlink(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::isDirectory() const {
	return std::filesystem::is_directory(myFile);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::isDevice() const {
#ifdef TARGET_WIN32
	return false;
#else
	return std::filesystem::status(myFile).type() == std::filesystem::block_file;
#endif
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::isHidden() const {
#ifdef TARGET_WINDOWS
#else
	return myFile.filename().string()[0] == '.';
#endif
}

//------------------------------------------------------------------------------------------------------------
void ofFile::setWriteable(bool flag){
	setReadOnly(!flag);
}

//------------------------------------------------------------------------------------------------------------
void ofFile::setReadOnly(bool flag){
	try{
		if(flag){
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::remove_perms);
		}else{
			std::filesystem::permissions(myFile,std::filesystem::perms::owner_write | std::filesystem::perms::add_perms);
		}
	}catch(std::exception & e){
		ofLogError() << "Couldn't set write permission on " << myFile << ": " << e.what();
	}
}

//------------------------------------------------------------------------------------------------------------
void ofFile::setExecutable(bool flag){
	try{
		std::filesystem::permissions(myFile, std::filesystem::perms::owner_exe | std::filesystem::perms::add_perms);
	}catch(std::exception & e){
		ofLogError() << "Couldn't set write permission on " << myFile << ": " << e.what();
	}
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::copyTo(string path, bool bRelativeToData, bool overwrite){
	if(isDirectory()){
		return ofDirectory(myFile).copyTo(path,bRelativeToData,overwrite);
	}
	if(path.empty()){
		ofLogError("ofFile") << "copyTo(): destination path is empty";
		return false;
	}
	if(!exists()){
		ofLogError("ofFile") << "copyTo(): source file does not exist";
		return false;
	}

	if(bRelativeToData){
		path = ofToDataPath(path);
	}
	if(ofFile::doesFileExist(path, bRelativeToData)){
		if(overwrite){
			ofFile::removeFile(path, bRelativeToData);
		}
		else{
			ofLogWarning("ofFile") << "copyTo(): destination file \"" << path << "\" already exists, set bool overwrite to true if you want to overwrite it";
		}
	}

	try{
		if(!ofDirectory(ofFilePath::getEnclosingDirectory(path,bRelativeToData)).exists()){
			ofFilePath::createEnclosingDirectory(path, bRelativeToData);
		}
		std::filesystem::copy(myFile,path);
	}catch(std::exception & except){
		ofLogError("ofFile") <<  "copyTo(): unable to copy \"" << path << "\":" << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::moveTo(string path, bool bRelativeToData, bool overwrite){
	if(path.empty()){
		ofLogError("ofFile") << "moveTo(): destination path is empty";
		return false;
	}
	if(!exists()){
		ofLogError("ofFile") << "moveTo(): source file does not exist";
		return false;
	}

	if(bRelativeToData){
		path = ofToDataPath(path);
	}
	if(ofFile::doesFileExist(path, bRelativeToData)){
		if(overwrite){
			ofFile::removeFile(path, bRelativeToData);
		}
		else{
			ofLogWarning("ofFile") << "moveTo(): destination file \"" << path << "\" already exists, set bool overwrite to true if you want to overwrite it";
		}
	}

	try{
		if(!ofDirectory(ofFilePath::getEnclosingDirectory(path,bRelativeToData)).exists()){
			ofFilePath::createEnclosingDirectory(path, bRelativeToData);
		}
		std::filesystem::rename(myFile,path);
		myFile = path;
	}
	catch(std::exception & except){
		ofLogError("ofFile") << "moveTo(): unable to move \"" << path << "\":" << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::renameTo(string path, bool bRelativeToData, bool overwrite){
	return moveTo(path,bRelativeToData,overwrite);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::remove(bool recursive){
	if(myFile.string().empty()){
		ofLogError("ofFile") << "remove(): file path is empty";
		return false;
	}
	if(!exists()){
		ofLogError("ofFile") << "remove(): file does not exist";
		return false;
	}

	try{
		if(recursive){
			std::filesystem::remove_all(myFile);
		}else{
			std::filesystem::remove(myFile);
		}
	}catch(std::exception & except){
		ofLogError("ofFile") << "remove(): unable to remove \"" << myFile << "\": " << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
uint64_t ofFile::getSize() const {
	try{
		return std::filesystem::file_size(myFile);
	}catch(std::exception & except){
		ofLogError("ofFile") << "getSize(): unable to get size of \"" << myFile << "\": " << except.what();
		return 0;
	}
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator==(const ofFile & file) const {
	return getAbsolutePath() == file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator!=(const ofFile & file) const {
	return getAbsolutePath() != file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator<(const ofFile & file) const {
	return getAbsolutePath() < file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator<=(const ofFile & file) const {
	return getAbsolutePath() <= file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator>(const ofFile & file) const {
	return getAbsolutePath() > file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::operator>=(const ofFile & file) const {
	return getAbsolutePath() >= file.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
// ofFile Static Methods
//------------------------------------------------------------------------------------------------------------

bool ofFile::copyFromTo(string pathSrc, string pathDst, bool bRelativeToData,  bool overwrite){
	if(bRelativeToData){
		pathSrc = ofToDataPath(pathSrc);
	}
	if(bRelativeToData){
		pathDst = ofToDataPath(pathDst);
	}

	if(!ofFile::doesFileExist(pathSrc, bRelativeToData)){
		ofLogError("ofFile") << "copyFromTo(): source file/directory doesn't exist: \"" << pathSrc << "\"";
		return false;
	}

	if(ofFile::doesFileExist(pathDst, bRelativeToData)){
		if(overwrite){
			ofFile::removeFile(pathDst, bRelativeToData);
		}else{
			ofLogWarning("ofFile") << "copyFromTo(): destination file/directory \"" << pathSrc << "\"exists,"
			<< " set bool overwrite to true if you want to overwrite it";
		}
	}

	return ofFile(pathSrc).copyTo(pathDst);
}

//be careful with slashes here - appending a slash when moving a folder will causes mad headaches
//------------------------------------------------------------------------------------------------------------
bool ofFile::moveFromTo(string pathSrc, string pathDst, bool bRelativeToData, bool overwrite){
	if(bRelativeToData){
		pathSrc = ofToDataPath(pathSrc);
	}
	if(bRelativeToData){
		pathDst = ofToDataPath(pathDst);
	}

	if(!ofFile::doesFileExist(pathSrc, bRelativeToData)){
		ofLogError("ofFile") << "moveFromTo(): source file/directory doesn't exist: \"" << pathSrc << "\"";
		return false;
	}

	if(ofFile::doesFileExist(pathDst, bRelativeToData)){
		if(overwrite){
			ofFile::removeFile(pathDst, bRelativeToData);
		}
		else{
			ofLogWarning("ofFile") << "moveFromTo(): destination file/folder \"" << pathDst << "\" exists,"
			<< " set bool overwrite to true if you want to overwrite it";
		}
	}

	return ofFile(pathSrc).moveTo(pathDst);
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::doesFileExist(string fPath,  bool bRelativeToData){
	if(bRelativeToData){
		fPath = ofToDataPath(fPath);
	}
	ofFile file(fPath);
	return !fPath.empty() && file.exists();
}

//------------------------------------------------------------------------------------------------------------
bool ofFile::removeFile(string path, bool bRelativeToData){
	if(bRelativeToData){
		path = ofToDataPath(path);
	}
	return ofFile(path).remove();
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- ofDirectory
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


//----------------------------------------
// Simple example of a boolean function that can be used
// with ofRemove to remove items from vectors.
bool hiddenFile(ofFile file){
	return file.isHidden();
}

//----------------------------------------
// Advanced example of a class that has a boolean function
// that can be used with ofRemove to remove items from vectors.
class ExtensionComparator : public unary_function<ofFile, bool> {
	public:
		vector<string> * extensions;
		inline bool operator()(const ofFile & file){
			std::filesystem::path curPath(file.path());
			string curExtension = ofToLower(curPath.extension().string());
			return !ofContains(*extensions, curExtension);
		}
};

//------------------------------------------------------------------------------------------------------------
ofDirectory::ofDirectory(){
	showHidden = false;
}

//------------------------------------------------------------------------------------------------------------
ofDirectory::ofDirectory(const std::filesystem::path & path){
	showHidden = false;
	open(path);
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::open(const std::filesystem::path & path){
	originalDirectory = ofFilePath::getPathForDirectory(path.string());
	files.clear();
	myDir = std::filesystem::path(ofToDataPath(originalDirectory));
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::close(){
	myDir = std::filesystem::path();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::create(bool recursive){

	if(!myDir.string().empty()){
		try{
			if(recursive){
				std::filesystem::create_directories(myDir);
			}else{
				std::filesystem::create_directory(myDir);
			}
		}
		catch(std::exception & except){
			ofLogError("ofDirectory") << "create(): " << except.what();
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::exists() const {
	return std::filesystem::exists(myDir);
}

//------------------------------------------------------------------------------------------------------------
string ofDirectory::path() const {
	return myDir.string();
}

//------------------------------------------------------------------------------------------------------------
string ofDirectory::getAbsolutePath() const {
	return std::filesystem::absolute(myDir).string();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::canRead() const {
	return ofFile(myDir,ofFile::Reference).canRead();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::canWrite() const {
	return ofFile(myDir,ofFile::Reference).canWrite();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::canExecute() const {
	return ofFile(myDir,ofFile::Reference).canExecute();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::isHidden() const {
	return ofFile(myDir,ofFile::Reference).isHidden();
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::setWriteable(bool flag){
	return ofFile(myDir,ofFile::Reference).setWriteable(flag);
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::setReadOnly(bool flag){
	return ofFile(myDir,ofFile::Reference).setReadOnly(flag);
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::setExecutable(bool flag){
	return ofFile(myDir,ofFile::Reference).setExecutable(flag);
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::setShowHidden(bool showHidden){
	this->showHidden = showHidden;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::isDirectory() const {
	return std::filesystem::is_directory(myDir);
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::copyTo(string path, bool bRelativeToData, bool overwrite){
	if(myDir.string().empty()){
		ofLogError("ofDirectory") << "copyTo(): source path is empty";
		return false;
	}
	if(!std::filesystem::exists(myDir)){
		ofLogError("ofDirectory") << "copyTo(): source directory does not exist";
		return false;
	}
	if(!std::filesystem::is_directory(myDir)){
		ofLogError("ofDirectory") << "copyTo(): source path is not a directory";
		return false;
	}

	if(bRelativeToData){
		path = ofToDataPath(path, bRelativeToData);
	}

	if(ofDirectory::doesDirectoryExist(path, bRelativeToData)){
		if(overwrite){
			ofDirectory::removeDirectory(path, true, bRelativeToData);
		}else{
			ofLogWarning("ofDirectory") << "copyTo(): dest \"" << path << "\" already exists, set bool overwrite to true to overwrite it";
			return false;
		}
	}

	ofDirectory(path).create(true);
	// Iterate through the source directory
	for(std::filesystem::directory_iterator file(myDir); file != std::filesystem::directory_iterator(); ++file){
		auto currentPath = std::filesystem::absolute(file->path());
		auto dst = std::filesystem::path(path) / currentPath.filename();
		if(std::filesystem::is_directory(currentPath)){
			ofDirectory current(currentPath);
			// Found directory: Recursion
			if(!current.copyTo(dst.string(),false)){
				return false;
			}
		}else{
			ofFile(file->path()).copyTo(dst.string(),false);
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::moveTo(string path, bool bRelativeToData, bool overwrite){
	if(copyTo(path,bRelativeToData,overwrite)){
		return remove(true);
	}else{
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::renameTo(string path, bool bRelativeToData, bool overwrite){
	return moveTo(path, bRelativeToData, overwrite);
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::remove(bool recursive){
	if(path().empty() || !std::filesystem::exists(myDir)){
		return false;
	}

	try{
		if(recursive){
			std::filesystem::remove_all(myDir);
		}else{
			std::filesystem::remove(myDir);
		}
	}catch(std::exception & except){
		ofLogError("ofDirectory") << "remove(): unable to remove file/directory: " << except.what();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::allowExt(string extension){
	if(extension == "*"){
		ofLogWarning("ofDirectory") << "allowExt(): wildcard extension * is deprecated";
	}
	extensions.push_back(ofToLower(extension));
}

//------------------------------------------------------------------------------------------------------------
int ofDirectory::listDir(string directory){
	open(directory);
	return listDir();
}

//------------------------------------------------------------------------------------------------------------
int ofDirectory::listDir(){
	files.clear();
	if(path().empty()){
		ofLogError("ofDirectory") << "listDir(): directory path is empty";
		return 0;
	}
	if(!std::filesystem::exists(myDir)){
		ofLogError("ofDirectory") << "listDir:() source directory does not exist: \"" << myDir << "\"";
		return 0;
	}
	
	// File::list(vector<File>) is broken on windows as of march 23, 2011...
	// so we need to use File::list(vector<string>) and build a vector<File>
	// in the future the following can be replaced width: cur.list(files);
	vector<string>fileStrings;
	std::filesystem::directory_iterator end_iter;
	if ( std::filesystem::exists(myDir) && std::filesystem::is_directory(myDir)){
		for( std::filesystem::directory_iterator dir_iter(myDir) ; dir_iter != end_iter ; ++dir_iter){
			files.emplace_back(dir_iter->path().string(), ofFile::Reference);
		}
	}else{
		ofLogError("ofDirectory") << "listDir:() source directory does not exist: \"" << myDir << "\"";
		return 0;
	}

	if(!showHidden){
		ofRemove(files, hiddenFile);
	}

	if(!extensions.empty() && !ofContains(extensions, (string)"*")){
		ExtensionComparator extensionFilter;
		extensionFilter.extensions = &extensions;
		ofRemove(files, extensionFilter);
	}

	if(ofGetLogLevel() == OF_LOG_VERBOSE){
		for(int i = 0; i < (int)size(); i++){
			ofLogVerbose() << "\t" << getName(i);
		}
		ofLogVerbose() << "listed " << size() << " files in \"" << originalDirectory << "\"";
	}

	return size();
}

//------------------------------------------------------------------------------------------------------------
string ofDirectory::getOriginalDirectory(){
	return originalDirectory;
}

//------------------------------------------------------------------------------------------------------------
string ofDirectory::getName(unsigned int position){
	return files.at(position).getFileName();
}

//------------------------------------------------------------------------------------------------------------
string ofDirectory::getPath(unsigned int position){
	return originalDirectory + getName(position);
}

//------------------------------------------------------------------------------------------------------------
ofFile ofDirectory::getFile(unsigned int position, ofFile::Mode mode, bool binary){
	ofFile file = files[position];
	file.changeMode(mode, binary);
	return file;
}

ofFile ofDirectory::operator[](unsigned int position){
	return getFile(position);
}

//------------------------------------------------------------------------------------------------------------
const vector<ofFile> & ofDirectory::getFiles() const{
	if(files.empty()){
		const_cast<ofDirectory*>(this)->listDir();
	}
	return files;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::getShowHidden(){
	return showHidden;
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::reset(){
	close();
}

//------------------------------------------------------------------------------------------------------------
static bool natural(const ofFile& a, const ofFile& b) {
	string aname = a.getBaseName(), bname = b.getBaseName();
	int aint = ofToInt(aname), bint = ofToInt(bname);
	if(ofToString(aint) == aname && ofToString(bint) == bname) {
		return aint < bint;
	} else {
		return a < b;
	}
}

//------------------------------------------------------------------------------------------------------------
void ofDirectory::sort(){
	ofSort(files, natural);
}

//------------------------------------------------------------------------------------------------------------
unsigned int ofDirectory::size(){
	return files.size();
}

//------------------------------------------------------------------------------------------------------------
int ofDirectory::numFiles(){
	return size();
}

//------------------------------------------------------------------------------------------------------------
// ofDirectory Static Methods
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::removeDirectory(string path, bool deleteIfNotEmpty, bool bRelativeToData){
	if(bRelativeToData){
		path = ofToDataPath(path);
	}
	return ofFile(path).remove(deleteIfNotEmpty);
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::createDirectory(string dirPath, bool bRelativeToData, bool recursive){
	if(bRelativeToData){
		dirPath = ofToDataPath(dirPath);
	}

	bool success = false;
	try{
		if(!recursive){
			success = std::filesystem::create_directory(dirPath);
		}else{
			success = std::filesystem::create_directories(dirPath);
		}
	}
	catch(std::exception & except){
		ofLogError("ofDirectory") << "createDirectory(): couldn't create directory \"" << dirPath << "\": " << except.what();
		return false;
	}

	if(!success){
		ofLogWarning("ofDirectory") << "createDirectory(): directory already exists: \"" << dirPath << "\"";
		success = true;
	}

	return success;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::doesDirectoryExist(string dirPath, bool bRelativeToData){
	if(bRelativeToData){
		dirPath = ofToDataPath(dirPath);
	}
	return std::filesystem::exists(dirPath);
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::isDirectoryEmpty(string dirPath, bool bRelativeToData){
	if(bRelativeToData){
		dirPath = ofToDataPath(dirPath);
	}

	if(!dirPath.empty() && std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)){
		return std::filesystem::directory_iterator(dirPath) == std::filesystem::directory_iterator();
	}
	return false;
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator==(const ofDirectory & dir){
	return getAbsolutePath() == dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator!=(const ofDirectory & dir){
	return getAbsolutePath() != dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator<(const ofDirectory & dir){
	return getAbsolutePath() < dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator<=(const ofDirectory & dir){
	return getAbsolutePath() <= dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator>(const ofDirectory & dir){
	return getAbsolutePath() > dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
bool ofDirectory::operator>=(const ofDirectory & dir){
	return getAbsolutePath() >= dir.getAbsolutePath();
}

//------------------------------------------------------------------------------------------------------------
vector<ofFile>::const_iterator ofDirectory::begin() const{
	return getFiles().begin();
}

//------------------------------------------------------------------------------------------------------------
vector<ofFile>::const_iterator ofDirectory::end() const{
	return files.end();
}

//------------------------------------------------------------------------------------------------------------
vector<ofFile>::const_reverse_iterator ofDirectory::rbegin() const{
	return getFiles().rbegin();
}

//------------------------------------------------------------------------------------------------------------
vector<ofFile>::const_reverse_iterator ofDirectory::rend() const{
	return files.rend();
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// -- ofFilePath
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------
string ofFilePath::addLeadingSlash(string path){
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty()){
		if(ofToString(path[0]) != sep.string()){
			path = (sep / path).string();
		}
	}
	return path;
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::addTrailingSlash(string path){
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty()){
		if(ofToString(path.back()) != sep.string()){
			path = (path / sep).string();
		}
	}
	return path;
}


//------------------------------------------------------------------------------------------------------------
string ofFilePath::getFileExt(string filename){
	return ofFile(filename,ofFile::Reference).getExtension();
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::removeExt(string filename){
	return ofFile(filename,ofFile::Reference).getBaseName();
}


//------------------------------------------------------------------------------------------------------------
string ofFilePath::getPathForDirectory(string path){
	// if a trailing slash is missing from a path, this will clean it up
	// if it's a windows-style "\" path it will add a "\"
	// if it's a unix-style "/" path it will add a "/"
	auto sep = std::filesystem::path("/").make_preferred();
	if(!path.empty() && ofToString(path.back())!=sep.string()){
		return (std::filesystem::path(path) / sep).string();
	}else{
		return path;
	}
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::removeTrailingSlash(string path){
	if(path.length() > 0 && (path[path.length() - 1] == '/' || path[path.length() - 1] == '\\')){
		path = path.substr(0, path.length() - 1);
	}
	return path;
}


//------------------------------------------------------------------------------------------------------------
string ofFilePath::getFileName(string filePath, bool bRelativeToData){
	if(bRelativeToData){
		filePath = ofToDataPath(filePath);
	}

	return std::filesystem::path(filePath).filename().string();
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::getBaseName(string filePath){
	return ofFile(filePath).getBaseName();
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::getEnclosingDirectory(string filePath, bool bRelativeToData){
	if(bRelativeToData){
		filePath = ofToDataPath(filePath);
	}
	return std::filesystem::path(filePath).parent_path().string();
}

//------------------------------------------------------------------------------------------------------------
bool ofFilePath::createEnclosingDirectory(string filePath, bool bRelativeToData, bool bRecursive) {
	return ofDirectory::createDirectory(ofFilePath::getEnclosingDirectory(filePath), bRelativeToData, bRecursive);
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::getAbsolutePath(string path, bool bRelativeToData){
	if(bRelativeToData){
		path = ofToDataPath(path);
	}
	return std::filesystem::canonical(path).string();
}


//------------------------------------------------------------------------------------------------------------
bool ofFilePath::isAbsolute(string path){
	return std::filesystem::path(path).is_absolute();
}

//------------------------------------------------------------------------------------------------------------
string ofFilePath::getCurrentWorkingDirectory(){
	#ifdef TARGET_OSX
		char pathOSX[FILENAME_MAX];
		uint32_t size = sizeof(pathOSX);
		if(_NSGetExecutablePath(pathOSX, &size) != 0){
			ofLogError("ofFilePath") << "getCurrentWorkingDirectory(): path buffer too small, need size " <<  size;
		}
		string pathOSXStr = string(pathOSX);
		string pathWithoutApp;
		size_t found = pathOSXStr.find_last_of("/");
		pathWithoutApp = pathOSXStr.substr(0, found);
		return pathWithoutApp;
	#else
		return std::filesystem::current_path().string();
	#endif
}

string ofFilePath::join(string path1, string path2){
	return (std::filesystem::path(path1) / std::filesystem::path(path2)).string();
}

string ofFilePath::getCurrentExePath(){
	#if defined(TARGET_LINUX) || defined(TARGET_ANDROID)
		char buff[FILENAME_MAX];
		ssize_t size = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
		if (size == -1){
			ofLogError("ofFilePath") << "getCurrentExePath(): readlink failed with error " << errno;
		}
		else{
			buff[size] = '\0';
			return buff;
		}
	#elif defined(TARGET_OSX)
		char path[FILENAME_MAX];
		uint32_t size = sizeof(path);
		if(_NSGetExecutablePath(path, &size) != 0){
			ofLogError("ofFilePath") << "getCurrentExePath(): path buffer too small, need size " <<  size;
		}
		return path;
	#elif defined(TARGET_WIN32)
		vector<char> executablePath(MAX_PATH);
		DWORD result = ::GetModuleFileNameA(NULL, &executablePath[0], static_cast<DWORD>(executablePath.size()));
		if(result == 0) {
			ofLogError("ofFilePath") << "getCurrentExePath(): couldn't get path, GetModuleFileNameA failed";
		}else{
			return string(executablePath.begin(), executablePath.begin() + result);
		}
	#endif
	return "";
}

string ofFilePath::getCurrentExeDir(){
	return getEnclosingDirectory(getCurrentExePath(), false);
}

string ofFilePath::getUserHomeDir(){
	#ifdef TARGET_WIN32
		// getenv will return any Environent Variable on Windows
		// USERPROFILE is the key on Windows 7 but it might be HOME
		// in other flavours of windows...need to check XP and NT...
		return string(getenv("USERPROFILE"));
	#elif !defined(TARGET_EMSCRIPTEN)
		struct passwd * pw = getpwuid(getuid());
		return pw->pw_dir;
	#else
		return "";
	#endif
}
