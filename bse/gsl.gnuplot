dB(x)=20*log(abs(x))/log(10)	# logarithmische Filterdarstellung in dB (s/20/10/ for abs(x)^2)
s2z(s)=(1+s)/(1-s)		# s-Ebene -> z-Ebene (bilinear Trans.)
j={0,1}				# sqrt(-1)
Z(x)=exp({0,-1}*x)		# gnuplot laufvariable x fuer H(z)
set samples 4000		# more accuracy
