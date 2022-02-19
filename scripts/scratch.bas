function test4(string s)
	print "test 4: ", s
	s = s + "balls"
end function

function test5(int i)
	return i+i
end function

function main()
	dim string s
	s = "fart 1"
	test4(s)
	test4("fart 2")
	print s
	print test5(10)
end function