type tt
	dim i
end type

dim string[] ss
dim i
dim tt[] arr
dim tt t

block
	call push(ss, "fart")
	print ss[0]

	# call len(ss)
	let i = len(ss)
	print "len is" i
	
	let i = len("poopsicle")
	print "len is" i

	t.i = 1
end block