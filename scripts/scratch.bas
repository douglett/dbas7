type tt
	dim i
	dim string s
end type

dim tt t
dim tt t2

block
	let t.s = "fart"
	let t.s = "test" + t.s
	print t.s

	# let t = t2
end block