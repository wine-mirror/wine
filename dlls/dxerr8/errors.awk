BEGIN {
	print "/* Machine generated. Do not edit. */"
	print ""
	lines = 0
}
{ 
	split($0, array, FS)

	if (NF > 0 && length(array[1]) > 0) {
		lines++

		# save the first word is the names array
		names[lines] = array[1] 

		# create the WCHAR version of the name
		printf "static const WCHAR name%dW[] = { ", lines
		i = 1
		len = length(array[1]) + 1
		while (i < len) {
			printf "'%s',", substr(array[1],i,1)
			i++
		}
		print  "0 };"
	
		# create the CHAR version of the description
		printf "static const CHAR description%dA[] = \"", lines
		word = 2
		while (word < (NF + 1)) {
			printf "%s", array[word]
			if (word < NF )
				printf " "
			word++
		}
		print  "\";"
	
		# create the WCHAR version of the description
		printf "static const WCHAR description%dW[] = { ", lines
		word = 2
		while (word < (NF + 1)) {
			i = 1
			len = length(array[word]) + 1
			while (i < len) {
				printf "'%s',", substr(array[word],i,1)
				i++
			}
			if (word < NF )
				printf "' ',"
			word++
		}
		print  "0 };"
	}
}
END {
	print ""
	print "static const error_info info[] = {"

	i = 1 
	while ( i <= lines) { 
		printf "    { %s, \"%s\", name%dW, description%dA, description%dW },\n", 
			names[i], names[i], i, i,i 
		i++
	}

	print "};"
}
