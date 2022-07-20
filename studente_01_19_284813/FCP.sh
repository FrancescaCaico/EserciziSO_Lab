#!/bin/sh 

#file FCP.sh uso di piu file temporanei per le gerarchie. 
#controllo lasco accetta 1+j parametri con j>1 

if test $# -lt 3
then echo ERRORE NUMERO DEI PARAMETRI ERRATO; exit 1
fi 

#il primo deve essere un numero intero strettamente positivo Q 
Q=$1 

case $Q in 
*[!0-9]*) echo Errore : $Q non è un parametro valido; exit 2;; 
*) 
	if test $Q -eq 0 
	then echo ERRORE: $Q non è un parametro valido; exit 3
	fi ;; 
esac 

#a questo punto avendo salvato Q effettuo uno shift per controllare le gerarchie 
shift
 
for i 
do
	if test -d $i -a -x $i 
	then 
		case $i in 
		/*) ;; 
		*) echo ERRORE DIRECTORY NON IN NOME ASSOLUTO; exit 4;; 
		esac
	else 
	echo ERRORE : $i NON È UNA DIRECTORY; exit 5
	fi 
done

# effettuo il set e export della variabile PATH

PATH=`pwd`:$PATH 
export PATH 

#è organizzato in J fasi una per ogni gerarchia
#dato che ci serve sapere per ogni fase quanti sono e quali sono i file leggibili la cui lunghezza in parole sia uguale a Q 
#è opportuno creare J file uno per ogni gerarchia in modo da poter poi effettuare il controllo sull'uguaglianza del contenuto di ogni file con i file delle restanti gerarchie  
#setto una variabile n per contare 

n=1 


for ger 
do 
	#creo il file per questa gerarchia 
	> /tmp/Tempfile_$n 
	FCR.sh $ger $Q /tmp/Tempfile_$n
	
	#al termine ho trovato esattamente un numero di file pari al numero di linee che ci sono nel file temporaneo 
	NR=`wc -l < /tmp/Tempfile_$n` 
	echo Al termine della ricerca nella gerarchia $ger ho trovato $NR file leggibili con lunghezza in parole pari a $Q 
	
	# incremento n e vado avanti 
	n=`expr $n + 1` 
done


#al termine delle n-fasi deve controllare tutti i file della prima gerarchia a giro ed effettuare per ogni file delle altre gerarchie 
# se il contenuto è lo stesso oppure no 

#i file della gerarchia 1 stanno dentro file con n=1
#per ogni file della prima gerarchia
G1=$1 

echo Adesso visualizziamo quali sono i file della gerarchia $G1 che rispettano le specifiche che hanno lo stesso contenuto dei file validi delle altre gerarchie
#nel file temporaneo ci sono i nomi assoluti dei file validi. 
n=2
for H in `cat /tmp/Tempfile_1` 
do
	#impostiamo n a 2 --> la prima ci serve per il confronto 
	

	for h in `cat /tmp/Tempfile_$n` 
	do 
		#per vedere se ci sono delle differenze uso il comando diff, se diff torna come return value = 0 allora sono uguali 
		diff $h $H > /dev/null 2>&1 
		if test $? -eq 0 
		then 
		#sono uguali 
		echo I file $H e il file $h sono uguali 
		fi 
		#altrimenti non sono uguali e quindi non scrivo 
	
	done
	#avendo controllato i file della gerarchia non ci serve piu il suo file per cui lo eliminiamo 
	rm /tmp/Tempfile_$n 
	n=`expr $n + 1`
done

#da ultimo eliminiamo il file della Gerarchia G1 
rm /tmp/Tempfile_1 
echo 
echo -- HO FINITO TUTTO --









