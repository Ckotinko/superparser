#ifndef MISC_H
#define MISC_H

//https://stackoverflow.com/questions/2670816/how-can-i-use-the-compile-time-constant-line-in-a-string/2671100
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#endif // MISC_H
