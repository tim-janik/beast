\appendix

# Appendix

&nbsp;

## One-dimensional Cubic Interpolation

\def\V#1{V_{#1}}

\def\W#1{W_{#1}}

\def\w#1{w_{#1}}

With four sample values $\V0$, $\V1$, $\V2$ and $\V3$, cubic interpolation approximates
the curve segment connecting $\V1$ and $\V2$, by using the beginning and ending
slope, the curvature and the rate of curvature change to construct a cubic
polynomial.


The cubic polynomial starts out as:

(@a)	$f(x) = \w3 x^3 + \w2 x^2 + \w1 x + \w0$

Where $0 <= x <= 1$, specifying the sample value of the curve segment between
$\V1$ and $\V2$ to obtain.

To calculate the coefficients $\w0,…,\w3$, we set out the following conditions:

(@b)	$f(0)  = \V1$
(@c)	$f(1)  = \V2$
(@d)	$f'(0) = \V1'$
(@e)	$f'(1) = \V2'$

We obtain $\V1'$ and $\V2'$ from the respecting slope triangles:

(@f)	$\V1' = \frac {\V2 - \V0} {2}$
(@g)	$\V2' = \frac {\V3 - \V1} {2}$

With (@f) →  (@d) and (@g) →  (@e) we get:

(@h)	$f'(0) = \frac {\V2 - \V0} {2}$
(@i)	$f'(1) = \frac {\V3 - \V1} {2}$

The derivation of $f(x)$ is:

(@j)	$f'(x)  = 3 \w3 x^2 + 2 \w2 x + \w1$

From $x=0$ → (@a), i.e. (@b), we obtain $\w0$ and from $x=0$ →  (@j),
i.e. (@h), we obtain $\w1$. With $\w0$ and $\w1$ we can solve the
linear equation system formed by (@c) →  (@a) and (@e) →  (@j)
to obtain $\w2$ and $\w3$.

(@c_a_)	  (@c) →  (@a):	  	$\w3 +   \w2 + \frac {\V2 - \V0} {2} + \V1 = \V2$
(@e_j_)	  (@e) →  (@j):	  	$3 \w3 + 2 \w2 + \frac {\V2 - \V0} {2}    = \frac {\V3 - \V1} {2}$

With the resulting coefficients:

$$
\begin{aligned}
    \w0 &= \V1 &                                        &(initial\:value)           \\
    \w1 &= \frac{\V2 - \V0} {2} &                       &(initial\:slope)           \\
    \w2 &= \frac{-\V3 + 4 \V2 - 5 \V1 + 2 \V0} {2} &    &(initial\:curvature)       \\
    \w3 &= \frac{\V3 - 3 \V2 + 3 \V1 - \V0} {2} &       &(rate\:change\:of\:curvature)
\end{aligned}
$$

Reformulating (@a) to involve just multiplications and additions (eliminating power), we get:

(@k)	$f(x) = ((\w3 x + \w2) x + \w1) x + \w0$

Based on $\V0,…,\V3$, $\w0,…,\w3$ and (@k), we can now approximate all values of the
curve segment between $\V1$ and $\V2$.

However, for practical resampling applications where only a specific
precision is required, the number of points we need out of the curve
segment can be reduced to a finite amount.
Lets assume we require $n$ equally spread values of the curve segment,
then we can precalculate $n$ sets of $\W{0,…,3}[i]$, $i=[0,…,n]$, coefficients
to speed up the resampling calculation, trading memory for
computational performance. With $\w{0,…,3}$ in (@a):

$$
\begin{alignedat} {2}
	f(x) \  &= &   \frac{\V3 - 3 \V2 + 3 \V1 - \V0} 2 x^3 \  +	& \\
		    &  & \frac{-\V3 + 4 \V2 - 5 \V1 + 2 \V0} 2 x^2 \ +	& \\
		    &  &                     \frac{\V2 - \V0} 2 x \  +	& \\
		    &  &                                   V1   \ \ 	&
\end{alignedat}
$$

sorted for $\V0,…,\V4$, we have:

(@l) $$\begin{aligned}
 f(x) \  = \  & \V3 \  (0.5 x^3 - 0.5 x^2) \  +		& \\
	          & \V2 \  (-1.5 x^3 + 2 x^2 + 0.5 x) \  +	& \\
			  & \V1 \  (1.5 x^3 - 2.5 x^2 + 1) \  +	& \\
			  & \V0 \  (-0.5 x^3 + x^2 - 0.5 x)		&
\end{aligned}$$

With (@l) we can solve $f(x)$ for all $x = \frac i n$, where $i = [0, 1, 2, …, n]$ by
substituting $g(i) = f(\frac i n)$ with

(@m)	$g(i) = \V3 \W3[i] + \V2 \W2[i] + \V1 \W1[i] + \V0 \W0[i]$

and using $n$ precalculated coefficients $\W{0,…,3}$ according to:

$$
\begin{alignedat}{4}
        m      &= \frac i n                                \\
        \W3[i] &=&  0.5 m^3 & - & 0.5 m^2 &         &      \\
        \W2[i] &=& -1.5 m^3 & + &   2 m^2 & + 0.5 m &      \\
        \W1[i] &=&  1.5 m^3 & - & 2.5 m^2 &         & + 1  \\
        \W0[i] &=& -0.5 m^3 & + &     m^2 & - 0.5 m &
\end{alignedat}
$$

We now need to setup $\W{0,…,3}[0,…,n]$ only once, and are then able to
obtain up to $n$ approximation values of the curve segment between
$\V1$ and $\V2$ with four multiplications and three additions using (@m),
given $\V0,…,\V3$.


## Modifier Keys

There seems to be a lot of inconsistency in the behaviour of modifiers
(shift and/or control) with regards to GUI operations like selections
and drag and drop behaviour.

According to the Gtk+ implementation, modifiers relate to DND operations
according to the following list:

Table: GDK drag-and-drop modifier keys

Modifier            Operation       Note / X-Cursor
--------------- --- --------------- ---------------------
none             →  copy            (else move (else link))
`SHIFT`          →  move            `GDK_FLEUR`
`CTRL`           →  copy            `GDK_PLUS`, `GDK_CROSS`
`SHIFT+CTRL`     →  link            `GDK_UL_ANGLE`

Regarding selections, the following email provides a short summary:

> ~~~
> From: Tim Janik <timj@gtk.org>
> To: Hacking Gnomes <Gnome-Hackers@gnome.org>
> Subject: modifiers for the second selection
> Message-ID: <Pine.LNX.4.21.0207111747190.12292-100000@rabbit.birnet.private>
> Date: Thu, 11 Jul 2002 18:10:52 +0200 (CEST)
>
> hi all,
>
> in the course of reimplementing drag-selection for a widget,
> i did a small survey of modifier behaviour in other (gnome/
> gtk) programs and had to figure that there's no current
> standard behaviour to adhere to:
>
> for all applications, the first selection works as
> expected, i.e. press-drag-release selects the region
> (box) the mouse was draged over. also, starting a new
> selection without pressing any modifiers simply replaces
> the first one. differences occour when holding a modifier
> (shift or ctrl) when starting the second selection.
>
> Gimp:
> Shift upon button press:        the new seleciton is added to the existing one
> Ctrl upon button press:         the new selection is subtracted from the
>                                 existing one
> Shift during drag:              the selection area (box or circle) has fixed
>                                 aspect ratio
> Ctrl during drag:               the position of the initial button press
>                                 serves as center of the selected box/circle,
>                                 rather than the upper left corner
>
> Gnumeric:
> Shift upon button press:        the first selection is resized
> Ctrl upon button press:         the new seleciton is added to the existing one
>
> Abiword (selecting text regions):
> Shift upon button press:        the first selection is resized
> Ctrl upon button press:         triggers a compound (word) selection that
>                                 replaces the first selection
>
> Mozilla (selecting text regions):
> Shift upon button press:        the first selection is resized
>
> Nautilus:
> Shift or Ctrl upon buttn press: the new selection is added to or subtracted
>                                 from the first selection, depending on whether
>                                 the newly selected region was selected before.
>                                 i.e. implementing XOR integration of the newly
>                                 selected area into the first.
>
> i'm not pointing this out to start a flame war over what selection style
> is good or bad and i do realize that different applications have
> different needs (i.e. abiword does need compound selection, and
> the aspect-ratio/centering style for gimp wouldn't make too much
> sense for text), but i think for the benfit of the (new) users,
> there should me more consistency regarding modifier association
> with adding/subtracting/resizing/xoring to/from existing selections.
>
> ---
> ciaoTJ
> ~~~
