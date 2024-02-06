set CA_DIR=%~d0%~p0..\
set CA_COM="BKTurbo8.exe"
set CA_B2O="BKbin2obj.exe"

set ASSEMBLER=%CA_DIR%%CA_COM%
set BIN2OBJ=%CA_DIR%%CA_B2O%

%BIN2OBJ% -simg -e -c -lbmpLogo resources/logo.bmp logo
%BIN2OBJ% -simg -e -c -t -lbmpCursor resources/cursor.bmp cursor
%BIN2OBJ% -simg -e -c -lbmpF resources/f.bmp f
%BIN2OBJ% -simg -e -c -lbmpQ resources/q.bmp q
%BIN2OBJ% -simg -e -c -lbmpUn resources/un.bmp un
%BIN2OBJ% -simg -e -c -lbmpB resources/b.bmp b
%BIN2OBJ% -simg -e -c -lbmp0 resources/0.bmp 0
%BIN2OBJ% -simg -e -c -lbmp1 resources/1.bmp 1
%BIN2OBJ% -simg -e -c -lbmp2 resources/2.bmp 2
%BIN2OBJ% -simg -e -c -lbmp3 resources/3.bmp 3
%BIN2OBJ% -simg -e -c -lbmp4 resources/4.bmp 4
%BIN2OBJ% -simg -e -c -lbmp5 resources/5.bmp 5
%BIN2OBJ% -simg -e -c -lbmp6 resources/6.bmp 6
%BIN2OBJ% -simg -e -c -lbmp7 resources/7.bmp 7
%BIN2OBJ% -simg -e -c -lbmp8 resources/8.bmp 8
%BIN2OBJ% -simg -e -c -lbmpGood resources/good.bmp good
%BIN2OBJ% -simg -e -c -lbmpBad resources/bad.bmp bad
%BIN2OBJ% -simg -e -c -lbmpWin resources/win.bmp win
%BIN2OBJ% -simg -e -c -lbmpN0 resources/n0.bmp n0
%BIN2OBJ% -simg -e -c -lbmpN1 resources/n1.bmp n1
%BIN2OBJ% -simg -e -c -lbmpN2 resources/n2.bmp n2
%BIN2OBJ% -simg -e -c -lbmpN3 resources/n3.bmp n3
%BIN2OBJ% -simg -e -c -lbmpN4 resources/n4.bmp n4
%BIN2OBJ% -simg -e -c -lbmpN5 resources/n5.bmp n5
%BIN2OBJ% -simg -e -c -lbmpN6 resources/n6.bmp n6
%BIN2OBJ% -simg -e -c -lbmpN7 resources/n7.bmp n7
%BIN2OBJ% -simg -e -c -lbmpN8 resources/n8.bmp n8
%BIN2OBJ% -simg -e -c -lbmpN9 resources/n9.bmp n9
%BIN2OBJ% -simg -e -c -lbmpBlock resources/block.bmp block

%ASSEMBLER% -l -o cl bk0010_miner.asm

%ASSEMBLER% -l -o li miner bk0010_miner.obj logo.obj cursor.obj f.obj q.obj un.obj b.obj 0.obj 1.obj 2.obj 3.obj 4.obj 5.obj 6.obj 7.obj 8.obj good.obj bad.obj win.obj n0.obj n1.obj n2.obj n3.obj n4.obj n5.obj n6.obj n7.obj n8.obj n9.obj block.obj

md out
copy *.bin out\*
copy *.lst out\*

del *.bin
del *.obj
del *.lst
