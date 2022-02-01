function test4(string s)
	print "test 4: ", s
	s = s + "balls"
end function

function main()
	dim string s
	s = "fart 1"
	test4(s)
	test4("fart 2")
	print s
end function