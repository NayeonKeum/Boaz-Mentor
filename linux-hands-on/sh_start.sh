i=0

for i in $(seq 1 5);
do
	touch t"$i".txt
done

echo "made "$i" files"

