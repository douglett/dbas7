function test4(string s)
	print "test 4: ", s
	s = s + "balls"
end function

function test5(int i)
	return i+i
end function

function test6()
	dim string s
	print "butt"
	if 0
		print "yes"
	else if 0
		print "yes2"
	else
		print "no"
	end if
	print "fart"
	input  s
	print "you typed '" s "'"
end function

function main()
	dim i
	i = 10
	if i == 1
		print "here"
	end if
	while i
		print "fart", i
		i = i - 1
		if i == 4
			break
		end if
	end while
end function