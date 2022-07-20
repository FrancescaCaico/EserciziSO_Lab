#!/bin/sh 
#FILE RICORSIVO 
 
# $1 LA DIRECTORY 
# $2 Q 
# $3 FILE TEMPORANEO
# MI SPOSTO NELLA DIR CORRENTE
cd $1 
NW=
#CERCO NELLA DIRECTORY CORRENTE TUTTI I FILE LEGGIBILI CON LUNGHEZZA IN PAROLE UGUALE A Q --> $2 
for H in * 
do 
	if test -f $H -a -r $H 
	then 
	NW=`wc -w < $H` 
		#se non è vuoto allora controllo che NW sia uguale a Q 
		if test $NW -ne 0
		 then 
			if test  $NW -eq $2 
			then 
			#scrivo il nome assoluto del file nel file corrispondente alla gerarchia
			echo `pwd`/$H >> $3 
			fi 
		fi 
	fi 
done

# itero sulle restanti
for H in *
do
	if test -d $H -a -x $H 
	then 
	$0 `pwd`/$H $2 $3	
	fi  
done

