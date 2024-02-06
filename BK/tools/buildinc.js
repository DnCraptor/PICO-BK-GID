/*
   buildinc.js
   ������ ��� ��������������� ���������� ������ �������/������
   ��� �������� Visual C++ (VS 2005 Express � �����)

   ��� ��� ������������?
 1. �������� � ������� tools
 2. �������� � ��������� ������� pre-build event ���
	cscript.exe //D //NoLogo "$(ProjectDir)tools\buildinc.js" "$(ProjectDir)" "version.h", ������,
	����������� � ��������, ������ "version.h" ������ ���������� ��� ��� �����
 3. ������, ������ ��� ������ ������� �� Ctrl+F5, ���� ���� ������ ��� ��� �� ����� ������, pre-build event ����� ���������, ��������������, ������ ���� ���������� � build ����� ���������.

 ��, ������ � �������� ������������ ������������� JScript ��� ����� ����� � ��� �� ��� ��� ������� ����� ����� ��������. ��� ����� ��������� ������ /D
 cscript.exe //D //NoLogo "$(ProjectDir)Properties\buildinc.js"

 � � ���� ������� ����� ��� ��� ��� ����� ������������:
 debugger;

 ��, ��� ���������� ���� ������� ��� ������� ����� ��������� ���������� (���� �� ������ ����� ����� ����� ������ Visual Studio). ��������� ����� �� ��� (����� ���� ��� �� �� ������� ��������� ����) � ������������ �� ��������. ����� ���� ����� ��������� ������� ������ //D �� ���������� � �� ���������� ������ �� �� ������ �������� �������, � ��� ��� ������������� ���������� ��� �� ��������� �� ��� ��������.

*/

function IncrementBuild(strRCFileName, iBuild)
{
	objFileSystem = new ActiveXObject("Scripting.FileSystemObject");
/*
���������: 
OpenTextFile(<Filename>,<Iomode>,<Create>,<Format>)

����������: ��������� ��������� ���� � ���������� ������ "TextStream", ����������� �� ����. 

���������: 
<Filename> - ������, ���� � �����.
<Iomode> - ��������������, �����. ��������� ��������: 
1 - ������� ���� ������ ��� ������.
2 - ������� ���� ��� ������. ���� ���� ��� �����������, ��� ���������� ��������.
8 - ������� ���� ��� ����������. ���� ���� ��� �����������, ���������� ����� ������������ � ����� �����.
<Create> - ��������������, ������ (�����). ��������� ����, ���� �� �� ���������� (True), ��� ��� (False). �� ��������� - False.
<Format> - ��������������, �����. ��������� ��������: 
-2 - ������� ���� � �������, ������������ �������� �� ���������.
-1 - ������� ���� � ������� Unicode.
0 - ������� ���� � ������� ASCII (�� ���������).
*/
	var objStream = objFileSystem.OpenTextFile(strRCFileName, 1, false, -1);
/*
���������: 
CreateTextFile(<Filename>,<Overwrite>,<Unicode>)

����������: ������ ����� ��������� ���� � ���������� ������ "TextStream", ����������� �� ����. 

���������: 
<Filename> - ������, ���� � �����.
<Overwrite> - ��������������, ������ (�����). �������������� ����, ���� �� ���������� (True), ��� ��� (False). �� ��������� - False. ���� ������� False � ���� ���������� - ��������� ������.
<Unicode> - ��������������, ������ (�����). ���� � ������� Unicode (True), ��� ASCII (False). �� ��������� - False.
*/
	var objOutStream = objFileSystem.CreateTextFile(strRCFileName + ".new", true, true);
    //������ ���������
	var arrToSearch = [
		"^(\\s*\\#define\\s+VERSION_NUM\\s+)(\\d+)$",
		"^(\\s*\\#define\\s+REVISION_NUM\\s+)(\\d+)$"
	];
    //������ 
	var arrOfRegexp = [];
	for (var i = 0; i < arrToSearch.length; ++i)
	{
		arrOfRegexp[i] = new RegExp(arrToSearch[i], "");
	}

	var date = new Date();
	var strYear = "" + (date.getYear() % 100); //��� - ��������� 2 �����
	var month = "" + (date.getMonth() + 1); //����� ������
	var strMonth = (month<10) ? "0" + month : "" + month; //��������� �������� ������
	var day = date.getDate(); //����� ���
	var strDay = (day<10) ? "0" + day : "" + day; //��������� �������� ���
	var strRevision = strYear + strMonth; // + strDay;

	while (!objStream.AtEndOfStream) //���� �� ����� �����
	{
		var strLine = objStream.ReadLine(); //������ ��������� ������
		if (strLine != "") //���� ������ �� ������
			for (var i = 0; i < arrToSearch.length; ++i) //������������ � ������ ����������
			{
				if (arrOfRegexp[i].exec(strLine) != null) //���� ������� �������
				{
					switch (i)
					{
					case 0: {	//���� �������� ������ ������� - �� ��� �������
						strLine = RegExp.$1 + strRevision
						break;
						}
					case 1: {	//���� �������� ������ ������� - �� ��� ����� ������
						if (typeof(iBuild) == "undefined")
						{
							iBuild = parseInt(RegExp.$2, 10) + 1;
						}
						strLine = RegExp.$1 + iBuild
						break;
						}
					}
					break; //������� �� �����
				}
			}

		objOutStream.WriteLine(strLine);
	}
	objStream.Close();
	objOutStream.Close();

	// �������� ������ ���� �� �����
	objFileSystem.CopyFile(strRCFileName + ".new", strRCFileName);

	// ������� ��������� ����
	objFileSystem.DeleteFile(strRCFileName + ".new");

	return iBuild;
}

//debugger;
// �������� ���������
var args = WScript.Arguments;
var CurrentBuild = IncrementBuild(args(0) + args(1));


/*
cscript: ������ "����������� ����������� ���� ��� ���������� ����� ����� ".js""

��������: ��� ������� ������� js-����� �� ��������� ������ � ������� ������� cscript (��������, "cscript configure.js" ��� ���������� php) ���������� ������:
����������� ����������� ���� ��� ���������� ����� ����� ".js"

�������: �������� �������� ���������� ������ � ����������� js.
�������: ��������� cmd ��-��� ��������������, ���������
ASSOC .JS=JSFile
*/