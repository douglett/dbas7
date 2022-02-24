function main()
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