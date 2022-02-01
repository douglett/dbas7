dim i

function test()
	dim j
	j = 101
	print "test", j
end function

function test2(int j)
	print "test"
end function

function main()
	print "hello world"
	test()
	# test2(0)
end function