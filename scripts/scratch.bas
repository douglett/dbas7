function while_test()
	dim i
	dim j
	i = 5
	while i
		j = 5
		print "fart i", i
		while j
			if j == 3
				j = j - 1
				continue
			else if j == 1
				break
			end if
			print "butt   i: " i "   j: " j
			j = j - 1
		end while
		i = i - 1
	end while
end function


function for_test()
	dim i
	for i = 1 to 5
		print i
	end for
end function


function for_test2()
	dim i
	dim j
	for i = 1 to 5
		print "fart i", i
		for j = 1 to 5
			if j == 3
				continue 2
			else if j == 5
				break
			end if
			print "butt   i: " i "   j: " j
		end for
	end for
end function


function main()
	for_test2()
end function