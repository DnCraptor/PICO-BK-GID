﻿#pragma once
// необходимые мне функции, выковырянные из библиотеки DSPL–2.0
// т.к. я не сумел собрать из неё дллку под VS
// (из-за графической библиотеки GNUPLOT)

#include <cmath>
#if defined TARGET_WINXP
#define _USE_MATH_DEFINES
#include <math.h>
#else
#include <corecrt_math_defines.h>
#endif
#include <utility>

#define RES_OK                                0

#define ERROR_FFT_SIZE                        0x06062021
#define ERROR_FILTER_A0                       0x06090100
#define ERROR_FILTER_APPROX                   0x06090116
#define ERROR_FILTER_FT                       0x06090620
#define ERROR_FILTER_ORD                      0x06091518
#define ERROR_FILTER_ORD_BP                   0x06091519
#define ERROR_FILTER_RP                       0x06091816
#define ERROR_FILTER_RS                       0x06091819
#define ERROR_FILTER_TYPE                     0x06092025
#define ERROR_FILTER_WP                       0x06092316
#define ERROR_FILTER_WS                       0x06092319

#define ERROR_NEGATIVE                        0x14050701
#define ERROR_POLY_ORD                        0x16151518
#define ERROR_PTR                             0x16201800
#define ERROR_RESAMPLE_RATIO                  0x18051801
#define ERROR_RESAMPLE_FRAC_DELAY             0x18050604
#define ERROR_SIZE                            0x19092605
#define ERROR_SYM_TYPE                        0x19251320
#define ERROR_WIN_PARAM                       0x23091601
#define ERROR_WIN_SYM                         0x23091925
#define ERROR_WIN_TYPE                        0x23092025

enum class FIR_WINDOW
{
	BARTLETT,
	BARTLETT_HANNING,
	BLACKMAN,
	BLACKMAN_HARRIS,
	BLACKMAN_NUTTALL,
	FLAT_TOP,
	GAUSSIAN,
	HAMMING,
	HANNING,
	LANCZOS,
	NUTTALL,
	RECTANGULAR,
	COS,
	CHEBY,
	KAISER,
};

enum class FIR_FILTER
{
	NONE = 0,
	LOWPASS,    // ФНЧ
	HIGHPASS,   // ФВЧ
	BANDSTOP,   // ФПЗ
	BANDPASS    // ФПП
};

constexpr auto M_2PI = 2.0 * M_PI;


/*! ****************************************************************************
int polyval(double* a, int ord, double* x, int n, double* y)

Функция рассчитывает полином P_N(x)  N-ого порядка для вещественного
аргумента, заданного вектором `x`.
  P_N(x) = a_0 + a_1 \cdot x + a_2 \cdot x^2 +
  a_3 \cdot x^3 + ... a_N \cdot x^N.

Для расчета используется формула Горнера:
  P_N(x) = a_0 + x \cdot (a_1 + x \cdot (a_2 + \cdot
  ( \ldots x \cdot (a_{N-1} + x\cdot a_N) \ldots )))

param[in]  a
Указатель на вектор вещественных коэффициентов полинома.
Размер вектора `[ord+1 x 1]`.
Коэффициент `a[0]` соответствует коэффициенту полинома a_0.

param[in]  ord
Порядок полинома N.

param[in]  x
Указатель на вектор аргумента полинома.
Размер вектора `[n x 1]`.
Значения полинома будут расчитаны для всех значений аргумента вектора `x`.

param[in]  n
Размер вектора агрумента полинома.

param[out]  y
Указатель на значения полинома для аргумента `x`.
Размер вектора `[n x 1]`.
Память должна быть выделена.

return
`RES_OK` --- полином рассчитан успешно.
В противном случае  "код ошибки".

author Бахурин Сергей. www.dsplib.org
***************************************************************************** */
int polyval(const double *a, int ord, double *x, int n, double *y);

/*!*****************************************************************************
int farrow_lagrange(double *s, int n, double p, double q,
                     double frd, double **y, int *ny)
Передискретизация вещественного сигнала на основе
полиномиальной Лагранжевой интерполяции.

Данная функция осуществляет передискретизацию входного сигнала `s` в `p/q` раз
со смещением дробной задержки `frd`.

Для передискретизации используется
<a href = "http://ru.dsplib.org/content/resampling_lagrange/resampling_lagrange.html">
полиномиальная Лагранжева интерполяция
</a> (структура Фарроу для полиномиальной интерполяции).

param [in] s
Указатель на вектор входного вещественного сигнала.
Размер вектора `[n x 1]`.

param [in] n
Размер вектора входного сигнала.

param [in] p
Числитель коэффициента передискретизации. - целевая частота дискретизации

param [in] q
Знаменатель коэффициента передискретизации. - исходная частота дискретизации

param [in] frd
Значение смещения дробной задержки в пределах одного отсчета.
Значение должно быть от 0 до 1.

param [out] y
Указатель на адрес результата передискретизации.
По данному адресу будет произведено динамическое выделение памяти
для результата передискретизации.
Будет выделено памяти под `n*q/p` отсчетов выходного сигнала.
Данный указатель не может быть `NULL`.

param [in] ny
Указатель на переменную, в которую будет записан
размер вектора `(*y)` после выделения памяти.

return
`RES_OK` --- передискретизация рассчитана успешно.
В противном случае  "код ошибки".

author Бахурин Сергей. www.dsplib.org
***************************************************************************** */
int farrow_lagrange(double *s, int n, double p, double q, double frd,
                    double **y, int &ny);
/*! ****************************************************************************
 int int farrow_spline(double *s, int n, double p, double q, double frd,
                           double **y, int *ny)
Передискретизация вещественного сигнала на основе сплайн интерполяции.

Данная функция осуществляет передискретизацию
входного сигнала `s` в `p/q` раз со смещением дробной задержки `frd`.
Для передискретизации используются
<a href = "http://ru.dsplib.org/content/resampling_spline/resampling_spline.html">
кубические сплайны Эрмита
</a>
(структура Фарроу для для сплайн-интерполяции).


param [in] s
Указатель на вектор входного вещественного сигнала.
Размер вектора `[n x 1]`.

param [in] n
Размер вектора входного сигнала.

param [in] p
Числитель коэффициента передискретизации. - целевая частота дискретизации

param [in] q
Знаменатель коэффициента передискретизации. - исходная частота дискретизации

param [in] frd
Значение смещения дробной задержки в пределах одного отсчета.
Значение должно быть от 0 до 1.

param [out] y
Указатель на адрес результата передискретизации.
По данному адресу будет произведено динамическое выделение памяти
для результата передискретизации.
Будет выделено памяти под `n*q/p` отсчетов выходного сигнала.
Данный указатель не может быть `NULL`.

param [in] ny
Указатель на переменную, в которую будет записан
размер вектора `(*y)` после выделения памяти.

return
`RES_OK` --- передискретизация рассчитана успешно.
В противном случае  "код ошибки".

author Бахурин Сергей. www.dsplib.org
***************************************************************************** */
int farrow_spline(double *s, int n, double p, double q, double frd,
                  double **y, int &ny);

/*! ****************************************************************************
int DSPL_API  fir_linphase(int ord, double w0, double w1, int filter_type,
                               int win_type, double win_param, double* h)

Расчет коэффициентов линейно-фазового КИХ-фильтра
методом оконного взвешивания.

Функция рассчитывает коэффициенты передаточной характеристики
H(z) = \sum_{n = 0}^{ord} h_n z^{-n}

цифрового линейно-фазового КИХ-фильтра фильтра.

param[in]  ord
Порядок фильтра (количество элементов задержки).
Количество коэффициентов фильтра равно `ord+1`.


param[in]  w0
Нормированная частота среза ФНЧ или ФВЧ,
или левая частота среза для полосового и режекторного фильтра.


param[in]  w1
Правая частота среза полосового и режекторного фильтра.
Данный параметр игнорируется для ФНЧ и ФВЧ.
Частота `w1` должна быть больше `w0`.


param[in]  filter_type
Тип фильтра.
Данный параметр определяет тип фильтра
и может принимать одно из значений:
DSPL_FILTER_LPF   - фильтр нижних частот;
DSPL_FILTER_HPF   - фильтр верхних частот;
DSPL_FILTER_BPASS - полосовой фильтр;
DSPL_FILTER_BSTOP - режекторный фильтр.



param [in]  win_type
Тип оконной функции.
Может принимать одно из следующих значений:

-------------------------------------------------------------------------
Значение  win_type           |  Описание
-----------------------------|-------------------------------------------
 DSPL_WIN_BARTLETT           | Непараметрическое окно Бартлетта
-----------------------------|-------------------------------------------
 DSPL_WIN_BARTLETT_HANN      | Непараметрическое окно Бартлетта-Ханна
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN           | Непараметрическое окно Блэкмана
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN_HARRIS    | Непараметрическое окно Блэкмана-Харриса
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN_NUTTALL   | Непараметрическое окно Блэкмана-Натталла
-----------------------------|-------------------------------------------
 DSPL_WIN_CHEBY              | Параметрическое окно Дольф-Чебышева.
                             | Параметр  win_param  задает уровень
                             | боковых лепестков в дБ.
-----------------------------|-------------------------------------------
 DSPL_WIN_COS                | Непараметрическое косинус-окно
-----------------------------|-------------------------------------------
 DSPL_WIN_FLAT_TOP           | Непараметрическое окно с максимально
                             | плоской вершиной
-----------------------------|-------------------------------------------
 DSPL_WIN_GAUSSIAN           | Параметрическое окно Гаусса
-----------------------------|-------------------------------------------
 DSPL_WIN_HAMMING            | Непараметрическое окно Хемминга
-----------------------------|-------------------------------------------
 DSPL_WIN_HANN               | Непараметрическое окно Ханна
-----------------------------|-------------------------------------------
 DSPL_WIN_KAISER             | Параметрическое окно Кайзера
-----------------------------|-------------------------------------------
 DSPL_WIN_LANCZOS            | Непараметрическое окно Ланкзоса
-----------------------------|-------------------------------------------
 DSPL_WIN_NUTTALL            | Непараметрическое окно Натталла
-----------------------------|-------------------------------------------
 DSPL_WIN_RECT               | Непараметрическое прямоугольное окно
-------------------------------------------------------------------------


param [in]  win_param
Параметр окна.
Данный параметр применяется только для параметрических оконных функций.
Для непараметрических окон игнорируется.


param[out]  h
Указатель на вектор коэффициентов линейно-фазового КИХ-фильтра H(z).
Размер вектора `[ord+1 x 1]`.
Память должна быть выделена.


note
Для соблюдения условия линейной ФЧХ используются
только симметричные окна.
Расчет режекторного линейно-фазового КИХ-фильтра
(если `filter_type = DSPL_FILTER_BSTOP`) производится только
для фильтров чётного порядка `ord`.
В случае нечетного порядка `ord` функция вернет код ошибки `ERROR_FILTER_ORD`.


return
`RES_OK`
Фильтр рассчитан успешно.
В противном случае  "код ошибки".


author  Бахурин Сергей www.dsplib.org
***************************************************************************** */
int fir_linphase(int ord, double w0, double w1, FIR_FILTER filter_type, FIR_WINDOW win_type, bool symmetric, double win_param, double *h);

/******************************************************************************
 * Linear phase lowpass filter
 ******************************************************************************/
int fir_linphase_lpf(int ord, double wp, FIR_WINDOW win_type, bool symmetric, double win_param, double *h);


/*! ****************************************************************************
 int linspace(double x0, double x1, int n, int type, double* x)

 Функция заполняет массив линейно-нарастающими, равноотстоящими
значениями от `x0` до `x1`

Заполняет массив `x` длиной `n` значениями в диапазоне
от x_0 до x_1. Функция поддерживает два типа заполнения
в соответствии с параметром `type`:

Симметричное заполнение согласно выражению (параметр `type=DSPL_SYMMETRIC`):

x(k) = x_0 + k \cdot dx,
dx = \frac{x_1 - x_0}{n-1}, k = 0 \ldots n-1.

Периодическое заполнение (параметр `type=DSPL_PERIODIC`) согласно выражению:

x(k) = x_0 + k \cdot dx,
dx = \frac{x_1 - x_0}{n}, k = 0 \ldots n-1.

param[in] x0
Начальное показателя x_0.

param[in] x1
Конечное значение x_1.

param[in] n
Количество точек массива `x`.

param[in] symmetric
Тип заполнения:

true --- симметричное заполнение,
false --- периодическое заполнение.

param[in,out] x
Указатель на вектор равноотстоящих значений .
Размер вектора `[n x 1]`.
Память должна быть выделена.

return
`RES_OK` --- функция выполнена успешно.
В противном случае  "код ошибки".

note
Отличие периодического и симметричного заполнения можно
понять из следующих примеров.
Пример 1. Периодическое заполнение.

    double x[5];
    linspace(0, 5, 5, false, x);

В массиве `x` будут лежать значения:
0,    1,    2,    3,    4


Пример 2. Симметричное заполнение.

        double x[5];
        linspace(0, 5, 5, true, x);

В массиве `x` будут лежать значения:
0,    1.25,    2.5,    3.75,    5


author
Бахурин Сергей
www.dsplib.org
***************************************************************************** */
int linspace(double x0, double x1, int n, bool symmetric, double *x);

/*! ****************************************************************************
int sinc(double* x, int n, double a, double* y)
Функция \textrm{sinc}(x,a) = \frac{\sin(ax)}{ax}.

Функция рассчитывает значения функции для вещественного вектора `x`.

param[in]  x
Указатель на вектор переменной  x .
Размер вектора `[n x 1]`.
Память должна быть выделена.

param[in]  n
Размер входного вектора `x`.

param[in]  a
Параметр функции  \textrm{sinc}(x,a) = \frac{\sin(ax)}{ax}.


param[out]  y
Указатель на вектор значений функции.
 Размер вектора `[n x 1]`.
 Память должна быть выделена.


return
`RES_OK` --- расчёт произведен успешно.
В противном случае  "код ошибки".

author Бахурин Сергей www.dsplib.org
***************************************************************************** */
int sinc(double *x, int n, double a, double *y);

/*! ****************************************************************************
int window(double* w, int n, int win_type, double param)
Расчет функции оконного взвешивания

Функция рассчитывает периодическую или симметричную оконную функцию
в соответствии с параметром `win_type`.

Периодическая оконная функция используется для спектрального анализа,
а симметричная оконная функция может быть использована для синтеза
КИХ-фильтров.

param [in,out] w
Указатель на вектор оконной функции.
Размер вектора `[n x 1]`.
Память должна быть выделена.
Рассчитанная оконная функция будет помещена по данному адресу.

param [in]  n
Размер вектора `w` оконной функции.

param [in]  win_type
Тип оконной функции.
Может принимать следующие значения:

-------------------------------------------------------------------------
Значение                     |  Описание
-----------------------------|-------------------------------------------
 DSPL_WIN_BARTLETT           | Непараметрическое окно Бартлетта
-----------------------------|-------------------------------------------
 DSPL_WIN_BARTLETT_HANN      | Непараметрическое окно Бартлетта-Ханна
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN           | Непараметрическое окно Блэкмана
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN_HARRIS    | Непараметрическое окно Блэкмана-Харриса
-----------------------------|-------------------------------------------
 DSPL_WIN_BLACKMAN_NUTTALL   | Непараметрическое окно Блэкмана-Натталла
-----------------------------|-------------------------------------------
 DSPL_WIN_CHEBY              | Параметрическое окно Дольф-Чебышева.
                             | Данное окно всегда является симметричным и
                             | игнорирует параметр  DSPL_WIN_SYM_MASK .
                             | Параметр  param  задает уровень боковых
                             | лепестков в дБ.
-----------------------------|-------------------------------------------
 DSPL_WIN_COS                | Непараметрическое косинус-окно
-----------------------------|-------------------------------------------
 DSPL_WIN_FLAT_TOP           | Непараметрическое окно с максимально
                             | плоской вершиной
-----------------------------|-------------------------------------------
 DSPL_WIN_GAUSSIAN           | Параметрическое окно Гаусса
-----------------------------|-------------------------------------------
 DSPL_WIN_HAMMING            | Непараметрическое окно Хемминга
-----------------------------|-------------------------------------------
 DSPL_WIN_HANN               | Непараметрическое окно Ханна
-----------------------------|-------------------------------------------
 DSPL_WIN_KAISER             | Параметрическое окно Кайзера
-----------------------------|-------------------------------------------
 DSPL_WIN_LANCZOS            | Непараметрическое окно Ланкзоса
-----------------------------|-------------------------------------------
 DSPL_WIN_NUTTALL            | Непараметрическое окно Натталла
-----------------------------|-------------------------------------------
 DSPL_WIN_RECT               | Непараметрическое прямоугольное окно
-------------------------------------------------------------------------

param [in]  symmetric
Задает симметричное или периодическое окно
true - симметричное, false - периодическое


param [in]  param
Параметр окна.
Данный параметр применяется только для параметрических оконных функций.
Для непараметрических окон игнорируется.

return
`RES_OK` если оконная функция рассчитана успешно.
В противном случае  "код ошибки".

author Бахурин Сергей. www.dsplib.org
***************************************************************************** */
int window(double *w, int n, FIR_WINDOW win_type, bool symmetric, double param);
/******************************************************************************
Barlett window function
*******************************************************************************/
int win_bartlett(double *w, int n, bool symmetric);
/******************************************************************************
Barlett - Hann    window function
******************************************************************************/
int win_bartlett_hann(double *w, int n, bool symmetric);
/******************************************************************************
Blackman    window function
******************************************************************************/
int win_blackman(double *w, int n, bool symmetric);
/******************************************************************************
Blackman - Harris window function
******************************************************************************/
int win_blackman_harris(double *w, int n, bool symmetric);
/******************************************************************************
Blackman - Nuttull     window function
******************************************************************************/
int win_blackman_nuttall(double *w, int n, bool symmetric);
/******************************************************************************
Chebyshev parametric window function
param sets spectrum sidelobes level in dB
ATTENTION! ONLY SYMMETRIC WINDOW
*******************************************************************************/
int win_cheby(double *w, int n, double param);
/******************************************************************************
Cosine window function
******************************************************************************/
int win_cos(double *w, int n, bool symmetric);
/******************************************************************************
Flat - Top     window function
******************************************************************************/
int win_flat_top(double *w, int n, bool symmetric);
/******************************************************************************
Gaussian window function
******************************************************************************/
int win_gaussian(double *w, int n, bool symmetric, double alpha);
/******************************************************************************
Hamming window function
******************************************************************************/
int win_hamming(double *w, int n, bool symmetric);
/******************************************************************************
Hann window function
******************************************************************************/
int win_hann(double *w, int n, bool symmetric);
/******************************************************************************
Kaiser window function
******************************************************************************/
int win_kaiser(double *w, int n, bool symmetric, double param);
/******************************************************************************
Lanczos window function
******************************************************************************/
int win_lanczos(double *w, int n, bool symmetric);
/******************************************************************************
Nuttall window function
******************************************************************************/
int win_nuttall(double *w, int n, bool symmetric);
/******************************************************************************
Rectangle window function
******************************************************************************/
int win_rect(double *w, int n);

/*! ****************************************************************************
int bessel_i0(double* x, int n, double* y)

Модифицированная функция Бесселя первого рода  I_0(x).

Функция рассчитывает значения функции для вещественного вектора `x`,
который должен принимать неотрицательные значения.

param[in]  x
Указатель на вектор переменной  x .
Размер вектора `[n x 1]`.
Память должна быть выделена.

param[in]  n
Размер входного вектора `x`.

param[out] y
Указатель на вектор значений функции  I_0(x).
Размер вектора `[n x 1]`.
Память должна быть выделена.

return
`RES_OK` --- расчёт произведен успешно.
В противном случае  "код ошибки".

note
Используемый алгоритм описан в статье:
Rational Approximations for the Modified Bessel Function
of the First Kind – I0(x) for Computations with Double Precision
by PAVEL HOLOBORODKO on NOVEMBER 11, 2015

author Бахурин Сергей www.dsplib.org
***************************************************************************** */
int bessel_i0(double *x, int n, double *y);
/*! ****************************************************************************
int cheby_poly1(double* x, int n, int ord, double* y)
Многочлен Чебышева первого рода порядка `ord`

Функция производит расчет многочлена Чебышева первого рода  C_{ord}(x) для
вещественного вектора `x` длины `n`на основе рекуррентной формулы

C_{ord}(x) = 2 x C_{ord-1}(x) - C_{ord-2}(x),

где  C_0(x) = 1 ,  C_1(x) = x

param[in] x
Указатель на вектор `x` аргумента полинома Чебышева первого рода.
Размер вектора `[n x 1]`.

param[in] n
Размер векторов `x` и `y`.

param[in] ord
Порядок полинома Чебышева первого рода.

param[out] y
Указатель на вектор значений полинома Чебышева,
соответствующих аргументу `x`.
Размер вектора `[n x 1]`.
Память должна быть выделена.

return
`RES_OK` Расчет произведен успешно.
В противном случае  "код ошибки".

author Бахурин Сергей www.dsplib.org
***************************************************************************** */
int cheby_poly1(double *x, int n, int ord, double *y);
