function main()
	dim i
	dim j
	i = 5
	while i
		j = 5
		while j
			print "butt   i: " i "   j: " j
			j = j - 1
			if j == 2
				# continue
			else if j == 1
				break
			end if
		end while
		i = i - 1
		print "fart i", i
	end while
end function