dim int[] a
dim string s
dim string[] ss

block
	call push(a, 101+2)
	print a[0]

	let s = "fart" + "butt"
	print s s

	call push(ss, "poop")
	call push(ss, "arse")
	print ss[0] ss[1]
end block