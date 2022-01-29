type tt
	dim i
end type

dim string[] ss
dim i
dim tt[] arr
dim tt t

block
	t.i = 101
	push(arr, t)
	print "here 1" arr[0].i
	
	t.i = 102
	arr[0] = t
	print "here 2" arr[0].i
end block