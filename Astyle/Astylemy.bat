SETLOCAL ENABLEDELAYEDEXPANSION
set START_DIR=%~d0%~p0

cd %START_DIR%..\

%START_DIR%astyle --style=break --indent=tab=4 --indent-classes --indent-switches --indent-preproc-define --indent-namespaces --max-instatement-indent=79 --break-blocks --pad-oper --pad-header --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name --break-closing-brackets --add-brackets --keep-one-line-statements --convert-tabs --suffix=none --recursive --exclude=dxsdk(jun2010) BK\*.cpp BK\*.h
%START_DIR%astyle --style=break --indent=tab=4 --indent-classes --indent-switches --indent-preproc-define --indent-namespaces --max-instatement-indent=79 --break-blocks --pad-oper --pad-header --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name --break-closing-brackets --add-brackets --keep-one-line-statements --convert-tabs --suffix=none --recursive BKDE\*.cpp BKDE\*.h BKDL\*.cpp BKDL\*.h HDDImgMaker\*.cpp HDDImgMaker\*.h ITCPC\*.cpp ITCPC\*.h BKHDDTool\*.cpp BKHDDTool\*.h shared\*.cpp shared\*.h BKTurbo8\*.cpp BKTurbo8\*.h

cd %START_DIR%



