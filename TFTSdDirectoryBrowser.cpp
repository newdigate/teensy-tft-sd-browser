//
// Created by Nicholas Newdigate on 02/02/2019.
//

#include "TFTSdDirectoryBrowser.h"

unsigned long TFTSdDirectoryBrowser::getTotalFileCount() {
  return _totalFiles;
}

void TFTSdDirectoryBrowser::initialize() {
    dirFile = _sd->open("/");

    while (openNext(file, dirFile)) {
        //Serial.println( file.name());
        if (!isHidden(file))
            _totalFiles++;

        file.close();
    }
    Serial.print("totalFiles:");
    Serial.println(_totalFiles, DEC);
}

void TFTSdDirectoryBrowser::reload() {
    Serial.println("reload()");
    rewind(dirFile);
    
   
    if (_currentPage > 0) {
        Serial.println("paging...");
        for (int i=0; i<_currentPage; i++){
            int nn = 0;
            while (nn < _nMaxFilesPerPage && openNext(file, dirFile)) {
                if (!isHidden(file)){

                    //Serial.print("skip:");
                    //file.printName(&Serial);
                    //Serial.println();
                    nn++;
                }
                file.close();
            }
        }
        Serial.println("paging done...");
    }

    Serial.println("cleanup...");
    if (false)
      for (int i=0; i<_nMaxFilesPerPage; i++) {
          if (filenames[i])
              delete [] filenames[i];
    }

    Serial.println("indexing...");
    int n = 0;
    int total = 0;
    while (n < _nMaxFilesPerPage && openNext(file, dirFile)) {
        // Skip hidden files.
        if (  !isHidden(file)){
            filenames[n] = new char[32];

            getFileName(file, filenames[n]);

            // Save dirIndex of file in directory.
            dirIndex[n] = total + (_currentPage * _nMaxFilesPerPage);
            n++;
        }
        total++;
        file.close();
    }
    _numFilesOnPage = n;
}

void TFTSdDirectoryBrowser::update() {
    _tft->fillScreen(_cFileBackground);
    for (int i=0; i<_numFilesOnPage; i++) {
        // Print the file number and name.
        if (i + _currentPage*_nMaxFilesPerPage == _selectedFileIndex) {

            //selected index
            //Serial.print(">>>");
            toggleSelection(i, true);

        } else {

            toggleSelection(i, false);
        }
    }
}

bool TFTSdDirectoryBrowser::isHidden(const File &file) {
#if USE_SD_H
    return false;
#else
    return file.isHidden();
#endif
}

void TFTSdDirectoryBrowser::rewind(File &file) {
#if USE_SD_H
    dirFile.rewindDirectory();
#else
    dirFile.rewind();
#endif
}

void TFTSdDirectoryBrowser::getFileName(File &file, char *buf) {
#if USE_SD_H
    strcpy(buf, file.name());
#else
    char *filename = new char[32] ;
    file.getName(filename, 32);
    strcpy(buf, filename);
    delete [] filename;
#endif
}

void TFTSdDirectoryBrowser::openFileIndex(File &file, File &dir, int index) {
#if USE_SD_H
    file = SD.open(filenames[index], FILE_READ);
#else
    file.open(&dir, dirIndex[index], O_RDONLY);
#endif
}

bool TFTSdDirectoryBrowser::isSubDir(File &file) {
#if USE_SD_H
    return file.isDirectory();
#else
    return file.isSubDir();
#endif
}

void TFTSdDirectoryBrowser::toggleSelection(int i, bool selected) {
    _tft->setCursor(1, i * 8 +1 );
    openFileIndex(file, dirFile, i);
    if (!isSubDir(file)) {
        _tft->setTextColor(
                selected? _cFileSelectedForeground : _cFileForeground,
                selected? _cFileSelectedBackground : _cFileBackground
        );
    } else
        _tft->setTextColor(
                selected? _cDirectorySelectedForeground : _cDirectoryForeground,
                selected? _cDirectorySelectedBackground : _cDirectoryBackground
        );
    _tft->print(filenames[i]);
    file.close();
}

void TFTSdDirectoryBrowser::setSelectFileIndex(int index) {
    if (_selectedFileIndex != index) {


        long newPage = index / _nMaxFilesPerPage;
        if (_currentPage != newPage) {
            _currentPage = newPage;
            _selectedFileIndex = index;
            reload();
            update();
        } else {
            toggleSelection(_selectedFileIndex % _nMaxFilesPerPage, false);
            _selectedFileIndex = index;
            toggleSelection(_selectedFileIndex % _nMaxFilesPerPage, true);
        }
    }
}

bool TFTSdDirectoryBrowser::openNext(File &file, File &dirFile) {
#if USE_SD_H
        file = dirFile.openNextFile(FILE_READ);
        return file;
#else
        return file.openNext(&dirFile, O_RDONLY);
#endif
}
