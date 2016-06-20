This simple project implements a quick and dirty x64 compiler for a very very small forth-based programming language.

It has been done in Windows, but should be relatively easy to port to other OSs.

Requirements:
  - [Go](golang.org)
  - [nasm](www.nasm.us)
  - [Visual Studio](https://www.visualstudio.com/)
The first two should be in your path, and vcvarsall.bat should have been called before attempting to build.
Then, just run `build_and_run.bat`

That should be all
