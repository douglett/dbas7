dim string[] ss
dim i

block
	call push(ss, "fart")
	print ss[0]

	# call len(ss)
	let i = len(ss)
	print "len is" i
	
	let i = len("poopsicle")
	print "len is" i
end block