type tt
	dim string s
end type

function test()
	dim j
	j = 101
	print "test", j
end function

function test2(int j)
	dim jj
	print "test" j
end function

function test3(tt t)
	print "test" t.s
end function

function test4(string s)
	print "test 4: ", s
end function

function main()
	dim tt t
	t.s = "fart"
	print "hello world"
	test()
	test2(123)
	test3(t)
	# test4(t.s)
end function