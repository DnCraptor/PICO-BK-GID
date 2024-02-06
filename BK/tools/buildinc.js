/*
   buildinc.js
   Скрипт для автоматического увеличения номера ревизии/сборки
   для проектов Visual C++ (VS 2005 Express и далее)

   Как его использовать?
 1. Засуньте в каталог tools
 2. Запишите в свойствах проекта pre-build event как
	cscript.exe //D //NoLogo "$(ProjectDir)tools\buildinc.js" "$(ProjectDir)" "version.h", причём,
	обязательно в кавычках, вместо "version.h" можете подставить своё имя файла
 3. Готово, правда при каждом нажатии на Ctrl+F5, даже если проект уже был до этого собран, pre-build event снова сработает, соответственно, скрипт тоже запустится и build снова произойдёт.

 Да, кстати — огромное преимущество использования JScript для таких задач — это то что эти скрипты очень легко отладить. Для этого добавляем ключик /D
 cscript.exe //D //NoLogo "$(ProjectDir)Properties\buildinc.js"

 И в теле скрипта пишем там где нам нужно остановиться:
 debugger;

 Всё, при достижении этой строчки нас спросят каким дебагером аттачиться (если на машине стоит более одной версии Visual Studio). Аттачимся любой из них (можно даже той же из которой запустили билд) и отлаживаемся на здоровье. Можно даже после окончания отладки ключик //D не выкидывать — на нормальную работу он не окажет никакого влияния, а вот при возникновении исключения тут же предложит всё это отладить.

*/

function IncrementBuild(strRCFileName, iBuild)
{
	objFileSystem = new ActiveXObject("Scripting.FileSystemObject");
/*
Синтаксис: 
OpenTextFile(<Filename>,<Iomode>,<Create>,<Format>)

Назначение: открывает текстовый файл и возвращает объект "TextStream", указывающий на него. 

Параметры: 
<Filename> - строка, путь к файлу.
<Iomode> - необязательный, число. Возможные значения: 
1 - Открыть файл только для чтения.
2 - Открыть файл для записи. Если файл уже существовал, его содержимое теряется.
8 - Открыть файл для добавления. Если файл уже существовал, информация будет дописываться в конец файла.
<Create> - необязательный, булево (число). Создавать файл, если он не существует (True), или нет (False). По умолчанию - False.
<Format> - необязательный, число. Возможные значения: 
-2 - Открыть файл в формате, используемом системой по умолчанию.
-1 - Открыть файл в формате Unicode.
0 - Открыть файл в формате ASCII (по умолчанию).
*/
	var objStream = objFileSystem.OpenTextFile(strRCFileName, 1, false, -1);
/*
Синтаксис: 
CreateTextFile(<Filename>,<Overwrite>,<Unicode>)

Назначение: создаёт новый текстовый файл и возвращает объект "TextStream", указывающий на него. 

Параметры: 
<Filename> - строка, путь к файлу.
<Overwrite> - необязательный, булево (число). Перезаписывать файл, если он существует (True), или нет (False). По умолчанию - False. Если указано False и файл существует - произойдёт ошибка.
<Unicode> - необязательный, булево (число). Файл в формате Unicode (True), или ASCII (False). По умолчанию - False.
*/
	var objOutStream = objFileSystem.CreateTextFile(strRCFileName + ".new", true, true);
    //список регекспов
	var arrToSearch = [
		"^(\\s*\\#define\\s+VERSION_NUM\\s+)(\\d+)$",
		"^(\\s*\\#define\\s+REVISION_NUM\\s+)(\\d+)$"
	];
    //создаём 
	var arrOfRegexp = [];
	for (var i = 0; i < arrToSearch.length; ++i)
	{
		arrOfRegexp[i] = new RegExp(arrToSearch[i], "");
	}

	var date = new Date();
	var strYear = "" + (date.getYear() % 100); //год - последние 2 цифры
	var month = "" + (date.getMonth() + 1); //номер месяца
	var strMonth = (month<10) ? "0" + month : "" + month; //строковое значение месяца
	var day = date.getDate(); //номер дня
	var strDay = (day<10) ? "0" + day : "" + day; //строковое значение дня
	var strRevision = strYear + strMonth; // + strDay;

	while (!objStream.AtEndOfStream) //пока не конец файла
	{
		var strLine = objStream.ReadLine(); //читаем очередную строку
		if (strLine != "") //если строка не пустая
			for (var i = 0; i < arrToSearch.length; ++i) //обрабатываем её нашими регекспами
			{
				if (arrOfRegexp[i].exec(strLine) != null) //если регексп подошёл
				{
					switch (i)
					{
					case 0: {	//если сработал первый регексп - то это ревизия
						strLine = RegExp.$1 + strRevision
						break;
						}
					case 1: {	//если сработал второй регексп - то это номер сборки
						if (typeof(iBuild) == "undefined")
						{
							iBuild = parseInt(RegExp.$2, 10) + 1;
						}
						strLine = RegExp.$1 + iBuild
						break;
						}
					}
					break; //выходим из цикла
				}
			}

		objOutStream.WriteLine(strLine);
	}
	objStream.Close();
	objOutStream.Close();

	// заменяем старый файл на новый
	objFileSystem.CopyFile(strRCFileName + ".new", strRCFileName);

	// удаляем временный файл
	objFileSystem.DeleteFile(strRCFileName + ".new");

	return iBuild;
}

//debugger;
// основная программа
var args = WScript.Arguments;
var CurrentBuild = IncrementBuild(args(0) + args(1));


/*
cscript: ошибка "Отсутствует исполняющее ядро для расширения имени файла ".js""

Проблема: при попытке запуска js-файла из командной строки с помощью команды cscript (например, "cscript configure.js" при компиляции php) появляется ошибка:
Отсутствует исполняющее ядро для расширения имени файла ".js"

Причина: нарушена файловая ассоциация файлов с расширением js.
Решение: запустить cmd из-под администратора, выполнить
ASSOC .JS=JSFile
*/