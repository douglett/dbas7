type tt
	dim a
	dim bbb
	dim string s
end type

dim tt a
dim string s

block
	let a.a = 101
	let a.bbb = 5
	let s = "hello-world"
	print "printing test" a.a  a.bbb  s

	let a.s = "farts"
	print s  a.s
end block