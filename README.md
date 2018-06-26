This is an experimental branch of fabcoin used for internal experimentation, research and development. 

Please do not deploy or use this branch to handle real transactions! 


To build, besides the dependencies, you need:
```
./autogen
./configure   --disable-bench --without-gui --disable-tests
```

Comments: The bench build is currently broken. Please compile without gui.


When adding new sources, you need to input them in the file 

```Makefile.am```

Make sure you test that the toolchain ```./autogen.sh->./configure->make``` works. 


