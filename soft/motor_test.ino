\documentclass[12pt,a4paper]{article}

\usepackage[T2A]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage[russian]{babel}
\usepackage{amsmath,amssymb}
\usepackage{graphicx}
\usepackage{booktabs}
\usepackage{array}
\usepackage{float}
\usepackage{caption}
\usepackage{xcolor}
\usepackage{geometry}
\usepackage{siunitx}
\usepackage{titlesec}
\usepackage{enumitem}a

\geometry{left=20mm,right=20mm,top=18mm,bottom=20mm}
\sisetup{
  output-decimal-marker={,},
  per-mode=symbol,
  exponent-product=\cdot,
  separate-uncertainty=true
}

\begin{document}

\begin{center}
{\large Московский физико-технический институт}\\[8mm]
{\Large\bfseries Отчёт о выполнении лабораторной работы 3.4.5}\\[3mm]
{\LARGE\bfseries Петля гистерезиса (динамический метод)}\\[9mm]
\end{center}

\begin{center}
Выполнил: Кокарев Константин\\[2mm]
Группа: Б01-502\\[2mm]
Дата: 30.09.26
\end{center}

\vspace{7mm}

\section{Аннотация}

В работе исследуются предельные петли гистерезиса и начальные кривые
намагничивания трёх ферромагнитных образцов: феррита, пермаллоя и
кремнистого железа. Петля гистерезиса наблюдается на осциллографе в режиме
$X$--$Y$: сигнал по горизонтальному каналу пропорционален напряжённости
магнитного поля $H$, а сигнал по вертикальному каналу после интегрирующей
$RC$-цепочки пропорционален магнитной индукции $B$. В конце работы проводится
калибровка каналов $X$ и $Y$ осциллографа и экспериментально определяется
постоянная времени интегрирующей $RC$-цепочки.

\section{Теоретические сведения}

Для тороидального образца напряжённость магнитного поля связана с током
в намагничивающей обмотке соотношением
\begin{equation}
    H=\frac{N_0 I_0}{l},
    \label{eq:Htor}
\end{equation}
где $N_0$ --- число витков намагничивающей обмотки, а $l=2\pi R$ ---
средняя длина магнитной линии. Напряжение $U_X$ снимается с резистора
$R_0$, поэтому
\begin{equation}
    U_X=R_0 I_0,
    \qquad
    H=\frac{N_0}{lR_0}U_X.
    \label{eq:Hux}
\end{equation}

ЭДС в измерительной обмотке пропорциональна производной магнитного потока.
Интегрирование сигнала выполняется $RC$-цепочкой. При условии
$\omega\tau\gg 1$, где $\tau=R_{\text{и}}C_{\text{и}}$, напряжение на
выходе интегратора связано с индукцией:
\begin{equation}
    B=\frac{\tau}{S N_{\text{и}}}U_Y.
    \label{eq:Buy}
\end{equation}

При периодическом перемагничивании зависимость $B(H)$ образует петлю
гистерезиса. Пересечение предельной петли с осью $H$ определяет
коэрцитивное поле $H_c$, с осью $B$ --- остаточную индукцию $B_r$.
Крайние точки частных петель при постепенном уменьшении амплитуды поля
лежат на начальной кривой намагничивания.

Если $K_x$ и $K_y$ --- чувствительности каналов осциллографа в
$\si{\milli\volt}/\text{дел}$, то цена деления в физических единицах:
\begin{equation}
    \alpha_H=\frac{N_0 K_x}{lR_0},
    \qquad
    \alpha_B=\frac{\tau K_y}{S N_{\text{и}}},
    \label{eq:alphas}
\end{equation}
где при подстановке в СИ $K_x$ и $K_y$ переводятся в
$\si{\volt\per дел}$. Тогда
\begin{equation}
\begin{aligned}
    H_{\max}&=\alpha_H\frac{2X_s}{2},&
    H_c&=\alpha_H\frac{2X_c}{2},\\
    B_s&=\alpha_B\frac{2Y_s}{2},&
    B_r&=\alpha_B\frac{2Y_r}{2}.
\end{aligned}
\label{eq:loopparams}
\end{equation}

\section{Методика измерений}

Осциллограф использовался в режиме $X$--$Y$. На канал $X$ подавалось
напряжение с резистора $R_0=\SI{0.2}{\ohm}$ в цепи намагничивающей
обмотки. На канал $Y$ подавалось напряжение с выхода интегрирующей
$RC$-цепочки. Для каждого образца сначала устанавливался ток, при котором
наблюдалась предельная петля гистерезиса, после чего фиксировались
$K_x$, $K_y$, $I_{\text{эф}}$, размеры петли $2X_s$, $2Y_s$, $2X_c$ и
$2Y_r$. Затем амплитуда намагничивающего тока плавно уменьшалась; по
крайним точкам частных петель снимались точки начальной кривой
намагничивания.

\section{Результаты измерений и обработка данных}

\subsection{Феррит}

Параметры образца:
\[
N_0=40,\qquad N_{\text{и}}=400,\qquad
S=\SI{3.0}{\centi\meter\squared},\qquad
l=\SI{25}{\centi\meter}.
\]

Для предельной петли были установлены
\[
K_x=\SI{20}{\milli\volt\per дел},\quad
K_y=\SI{20}{\milli\volt\per дел},\quad
I_{\text{эф}}=\SI{241}{\milli\ampere}.
\]
Измерено:
\[
2X_s=8,\quad 2Y_s=5,\quad 2X_c=0.8,\quad 2Y_r=2
\quad \text{(дел.)}.
\]
По формулам~\eqref{eq:alphas}--\eqref{eq:loopparams}:
\[
H_{\max}=\SI{64.0}{\ampere\per\meter},\quad
H_c=\SI{6.40}{\ampere\per\meter},
\]
\[
B_s=\SI{0.1667}{\tesla},\quad
B_r=\SI{0.0667}{\tesla}.
\]

Точки начальной кривой намагничивания:
\begin{table}[H]
\centering
\caption{Начальная кривая намагничивания феррита.}
\begin{tabular}{cSS}
\toprule
№ & {$H$, \si{\ampere\per\meter}} & {$B$, \si{\milli\tesla}}\\
\midrule
1&64.0&166.67\\
2&48.0&160.00\\
3&38.4&146.67\\
4&32.0&133.33\\
5&27.2&120.00\\
6&17.6&93.33\\
7&11.2&66.67\\
8&8.0&33.33\\
0&0&0\\
\bottomrule
\end{tabular}
\end{table}

\begin{figure}[H]
\centering
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{ferrite_initial.png}{0.95\linewidth}
{Место для графика начальной кривой намагничивания феррита}
\captionof{figure}{Начальная кривая намагничивания феррита.}
\end{minipage}\hfill
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{ferrite_hysteresis.jpg}{0.95\linewidth}
{Место для фотографии предельной петли гистерезиса феррита}
\captionof{figure}{Петля гистерезиса феррита.}
\end{minipage}
\end{figure}

\subsection{Пермаллой}

Параметры образца:
\[
N_0=20,\qquad N_{\text{и}}=300,\qquad
S=\SI{0.76}{\centi\meter\squared},\qquad
l=\SI{13.3}{\centi\meter}.
\]

Для предельной петли:
\[
K_x=\SI{50}{\milli\volt\per дел},\quad
K_y=\SI{50}{\milli\volt\per дел},\quad
I_{\text{эф}}=\SI{410}{\milli\ampere},
\]
\[
2X_s=6,\quad 2Y_s=3.2,\quad 2X_c=1.6,\quad 2Y_r=3.2.
\]
Получено:
\[
H_{\max}=\SI{112.77}{\ampere\per\meter},\quad
H_c=\SI{30.07}{\ampere\per\meter},
\]
\[
B_s=\SI{1.404}{\tesla},\quad
B_r=\SI{1.404}{\tesla}.
\]

\begin{table}[H]
\centering
\caption{Начальная кривая намагничивания пермаллоя.}
\begin{tabular}{cSS}
\toprule
№ & {$H$, \si{\ampere\per\meter}} & {$B$, \si{\milli\tesla}}\\
\midrule
1&75.18&1403.51\\
2&37.59&1403.51\\
3&24.0576&1122.81\\
4&24.0576&701.75\\
5&21.0504&421.05\\
6&16.5396&175.44\\
7&13.5324&70.18\\
8&0&0\\
\bottomrule
\end{tabular}
\end{table}

\begin{figure}[H]
\centering
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{permalloy_initial.png}{0.95\linewidth}
{Место для графика начальной кривой намагничивания пермаллоя}
\captionof{figure}{Начальная кривая намагничивания пермаллоя.}
\end{minipage}\hfill
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{permalloy_hysteresis.jpg}{0.95\linewidth}
{Место для фотографии предельной петли гистерезиса пермаллоя}
\captionof{figure}{Петля гистерезиса пермаллоя.}
\end{minipage}
\end{figure}

\subsection{Кремнистое железо}

Параметры образца:
\[
N_0=25,\qquad N_{\text{и}}=250,\qquad
S=\SI{2.0}{\centi\meter\squared},\qquad
l=\SI{11}{\centi\meter}.
\]

Для предельной петли:
\[
K_x=\SI{100}{\milli\volt\per дел},\quad
K_y=\SI{100}{\milli\volt\per дел},\quad
I_{\text{эф}}=\SI{1.272}{\ampere},
\]
\[
2X_s=8,\quad 2Y_s=2,\quad 2X_c=1.2,\quad 2Y_r=0.8.
\]
Получено:
\[
H_{\max}=\SI{454.4}{\ampere\per\meter},\quad
H_c=\SI{68.16}{\ampere\per\meter},
\]
\[
B_s=\SI{0.800}{\tesla},\quad
B_r=\SI{0.320}{\tesla}.
\]

\begin{table}[H]
\centering
\caption{Начальная кривая намагничивания кремнистого железа.}
\begin{tabular}{cSS}
\toprule
№ & {$H$, \si{\ampere\per\meter}} & {$B$, \si{\milli\tesla}}\\
\midrule
1&454.4&800\\
2&340.8&800\\
3&227.2&560\\
4&170.4&480\\
5&113.6&400\\
6&45.44&160\\
7&34.08&80\\
8&0&0\\
\bottomrule
\end{tabular}
\end{table}

\begin{figure}[H]
\centering
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{silicon_iron_initial.png}{0.95\linewidth}
{Место для графика начальной кривой намагничивания кремнистого железа}
\captionof{figure}{Начальная кривая намагничивания кремнистого железа.}
\end{minipage}\hfill
\begin{minipage}[t]{0.48\textwidth}
\centering
\imgorplaceholder{silicon_iron_hysteresis.jpg}{0.95\linewidth}
{Место для фотографии предельной петли гистерезиса кремнистого железа}
\captionof{figure}{Петля гистерезиса кремнистого железа.}
\end{minipage}
\end{figure}

\subsection{Калибровка канала $X$}

При закороченной намагничивающей обмотке ток имеет практически
синусоидальную форму. Амперметр измеряет действующее значение
$I_{\text{эф}}$, поэтому удвоенная амплитуда напряжения на $R_0$ равна
$2\sqrt{2}I_{\text{эф}}R_0$. Практическая чувствительность канала:
\begin{equation}
    K_x^{\text{кал}}=
    \frac{2\sqrt{2}\,I_{\text{эф}}R_0}{2x}.
    \label{eq:Kxcal}
\end{equation}

Для расчёта использовано $R_0=\SI{0.2}{\ohm}$ и $2x=10$ делений.

\begin{table}[H]
\centering
\caption{Калибровка горизонтального канала осциллографа.}
\begin{tabular}{S S S S}
\toprule
{$K_x$, \si{\milli\volt}/\text{дел}} &
{$I_{\text{эф}}$, \si{\ampere}} &
{$K_x^{\text{кал}}$, \si{\milli\volt}/\text{дел}} &
{отклонение, \%}\\
\midrule
100&1.6880&95.49&4.51\\
50&0.8300&46.95&6.10\\
20&0.3270&18.50&7.51\\
10&0.1570&8.88&11.19\\
5&0.0732&4.14&17.18\\
\bottomrule
\end{tabular}
\end{table}

Дополнительное измерение при установке $\SI{0.1}{\volt\per дел}$
дало $K_x^{\text{кал}}\approx\SI{91.36}{\milli\volt\per дел}$,
что также показывает заметное отличие реального масштаба от номинального.

\subsection{Калибровка канала $Y$}

Для синусоидального сигнала с действующим напряжением $U_{\text{эф}}$
удвоенная амплитуда равна $2\sqrt{2}U_{\text{эф}}$, поэтому
\begin{equation}
    K_y^{\text{кал}}=
    \frac{2\sqrt{2}\,U_{\text{эф}}}{2y}.
    \label{eq:Kycal}
\end{equation}

\begin{table}[H]
\centering
\caption{Калибровка вертикального канала осциллографа.}
\begin{tabular}{S S S S S}
\toprule
{$K_y$, \si{\milli\volt}/\text{дел}} &
{$2y$, дел.} &
{$U_{\text{эф}}$, \si{\volt}} &
{$K_y^{\text{кал}}$, \si{\milli\volt}/\text{дел}} &
{отклонение, \%}\\
\midrule
100&4.0&0.1340&94.75&5.25\\
50&8.0&0.1340&47.38&5.25\\
20&7.8&0.0480&17.41&12.97\\
10&8.0&0.0250&8.84&11.61\\
5&8.0&0.0134&4.74&5.25\\
\bottomrule
\end{tabular}
\end{table}

\subsection{Определение постоянной времени $RC$-цепочки}

Измеренные амплитуды:
\[
U_{\text{вх}}=\SI{1.6}{\volt},\qquad
U_{\text{вых}}=\SI{0.013}{\volt},\qquad
\nu=\SI{50}{\hertz},
\]
\[
\omega=2\pi\nu\approx\SI{314}{\per\second}.
\]
Для интегрирующей цепочки
\begin{equation}
    \tau_{\text{эксп}}=
    \frac{U_{\text{вх}}}{\omega U_{\text{вых}}}
    =
    \frac{1.6}{314\cdot 0.013}
    =
    \SI{0.392}{\second}.
    \label{eq:tauexp}
\end{equation}

По номиналам установки
\[
R_{\text{и}}=\SI{20}{\kilo\ohm},\qquad
C_{\text{и}}=\SI{20}{\micro\farad},
\]
поэтому
\begin{equation}
    \tau_{\text{теор}}=R_{\text{и}}C_{\text{и}}
    =\SI{0.400}{\second}.
\end{equation}
Расхождение составляет около $2.0\%$. Кроме того,
\[
\frac{1}{\omega}\approx\SI{3.18e-3}{\second},
\qquad
\omega\tau_{\text{эксп}}\approx123\gg1,
\]
то есть условие работы цепочки как интегратора выполняется.

\subsection{Оценка погрешностей}

Для величины, являющейся произведением степеней независимых параметров,
использовалась стандартная формула распространения погрешностей
\begin{equation}
    \left(\frac{\sigma_f}{f}\right)^2
    =
    \sum_i a_i^2
    \left(\frac{\sigma_{x_i}}{x_i}\right)^2,
    \qquad
    f=C\prod_i x_i^{a_i}.
    \label{eq:error-general}
\end{equation}

В исходной таблице не заданы отдельные погрешности геометрических
параметров тороидов и отсчёта координат по сетке осциллографа. Поэтому
численная оценка ниже учитывает доступные экспериментальные проверки:
отличие откалиброванной чувствительности каналов от номинальной и
расхождение $\tau_{\text{эксп}}$ с $R_{\text{и}}C_{\text{и}}$.

Для величин, определяемых по горизонтальной оси,
\begin{equation}
    \varepsilon_H \simeq
    \frac{|K_x^{\text{кал}}-K_x|}{K_x},
\end{equation}
а для величин, определяемых по вертикальной оси,
\begin{equation}
    \varepsilon_B \simeq
    \sqrt{
    \left(\frac{|K_y^{\text{кал}}-K_y|}{K_y}\right)^2+
    \left(\frac{|\tau_{\text{эксп}}-\tau_{\text{теор}}|}
    {\tau_{\text{теор}}}\right)^2
    }.
    \label{eq:error-B}
\end{equation}
Абсолютная погрешность вычислялась как
\begin{equation}
    \sigma_q=\varepsilon_q\,q.
\end{equation}

Для рабочих диапазонов получено:
\[
\begin{array}{c|cc}
\text{образец} & \varepsilon_H & \varepsilon_B\\ \hline
\text{феррит} & 7.5\% & 13.1\%\\
\text{пермаллой} & 6.1\% & 5.6\%\\
\text{кремнистое железо} & 4.5\% & 5.6\%
\end{array}
\]

Итоговые результаты с оценёнными погрешностями:
\begin{table}[H]
\centering
\caption{Магнитные характеристики исследованных образцов.}
\begin{tabular}{lcccc}
\toprule
Образец &
$H_{\max}$, \si{\ampere\per\meter} &
$H_c$, \si{\ampere\per\meter} &
$B_s$, \si{\tesla} &
$B_r$, \si{\tesla}\\
\midrule
Феррит &
$64\pm5$ &
$6.4\pm0.5$ &
$0.167\pm0.022$ &
$0.0667\pm0.0088$\\
Пермаллой &
$113\pm7$ &
$30.1\pm1.8$ &
$1.40\pm0.08$ &
$1.40\pm0.08$\\
Кремнистое железо &
$454\pm21$ &
$68.2\pm3.1$ &
$0.800\pm0.045$ &
$0.320\pm0.018$\\
\bottomrule
\end{tabular}
\end{table}

Для постоянной времени как оценку систематической погрешности можно
использовать расхождение с номинальным значением:
\[
\tau_{\text{эксп}}=(0.392\pm0.008)\,\si{\second}.
\]

\section{Обсуждение результатов и выводы}

В работе получены предельные петли гистерезиса и начальные кривые
намагничивания трёх ферромагнитных образцов. Наибольшая индукция
насыщения среди исследованных образцов получена для пермаллоя, тогда как
наибольшее коэрцитивное поле --- для кремнистого железа. Феррит имеет
наименьшее коэрцитивное поле и существенно меньшую индукцию насыщения.

Калибровка показала, что практические чувствительности каналов
осциллографа отличаются от номинальных; это отличие использовано как
основной вклад в оценку погрешностей магнитных характеристик. Для
вертикального канала дополнительно учтено расхождение экспериментальной
и номинальной постоянных времени интегрирующей цепочки.

Экспериментально найдено
$\tau_{\text{эксп}}\approx\SI{0.392}{\second}$, что отличается от
$R_{\text{и}}C_{\text{и}}=\SI{0.400}{\second}$ примерно на $2\%$.
Условие $\omega\tau\gg1$ выполняется с большим запасом, следовательно,
использование $RC$-цепочки для интегрирования сигнала в данном опыте
обосновано.

\end{document}
