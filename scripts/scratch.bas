type tt
	dim i
	dim string j
end type

dim i

function test()
	dim j
	j = 404
	print "test" i j
end function

function main()
	dim j
	let j = 101
	print "hello world" j
	test()
	print "hello world" j
	let i = 202
	test()
end function