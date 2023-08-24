# RDAndroid-Stream
Streaming webcam video to android phone using rawdrawandroid (https://github.com/cnlohr/rawdrawandroid ) on a local network. 

## To use this app
 * please visit CNLorh's rawdraw-android github: https://github.com/cnlohr/rawdrawandroid and get the demo code installed and working on your android device. 

 * Next replace the Makefile, AndroidManifest.xml, and strings.xml (strings.xml is located in the ./sources/res/values folder) of the rawdraw-android repo with the corresponding files in the Maze repo.
 
 * Move the remaining files to the rawdraw-android repo. 
 
 * Finally, in the new makefile you will need to change the value of `ANDROIDVERSION?=` to your version of Android. 

Good Luck!
