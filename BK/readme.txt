/*************************************************
*           BK Emulator v3.13 for Windows         *
*************************************************/

Written by: Yuriy Kalmykov <kalmykov@stoik.com>
            gid <gray-gid@yandex.ru>
    Copyright (c) 2002-2005 Yuriy Kalmykov
    Copyright (c) 2012-2023 gid

    BK Emulator is a program emulated hardware environment for running
code for BK 0010(01) in different configurations.

    This code may be used in compiled form in any way you desire.
This file or it's parts can't be redistributed without the authors
written consent, but can be modified for your private needs.
    Providing that this notice and the authors name and all copyright
notices remains intact.

    Please, an email me to know how you are using it and where. You can
ask me for any information about this below code or any attendant
knowledge.

    This file is provided "as is" with no expressed or implied warranty.
The author accepts no liability for any damage or loss of business that
this product may cause.


Команды:
/h или /? - эта справка.

/m имя_файла.msf - загрузка msf файла.

/b имя_файла.bin [/c имя_конф.][/p n0:n1][/s скрипт] - загрузка bin файла в
    заданную конфигурацию, с подключением страниц n0 в окно0 и n1 в окно1
    для конфигураций БК11(М), через свой скрипт.
    /c - задать конфигурацию, если ключ не указан, используется текущая
        конфигурация.
    /p - задать номера страниц, если ключ не указан, страницы не изменяются.
    /s - задать загрузочный скрипт, если ключ не указан, используется скрипт
        по умолчанию.

/d имя_файла [/c имя_конф.][/p n0:n1][/l адрес][/a адрес][/r][/f] - загрузка
    дампа в формате .bin в память.
    /c - задать конфигурацию, если ключ не указан, используется текущая
        конфигурация.
    /p - задать номера страниц, если ключ не указан, страницы не изменяются.
    /l - задать адрес загрузки дампа, если ключ не указан, адрес берётся из
        заголовка .bin.
    /a - задать адрес запуска, если ключ не указан, берётся адрес загрузки.
    /r - запустить загруженный дамп с адреса запуска, если ключ не указан,
        то после загрузки продолжается работа эмулятора с того места, где
        его прервали.
    /f - загружать произвольный файл, а не в формате .bin, адрес загрузки по 
        принимается - 0.

/t имя_файла{.tap|.wav} - загрузка tape или wave файла и запуск на
    воспроизведение.

/s скрипт - запуск файла скрипта.