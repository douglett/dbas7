type mytype
	dim i
	# dim int[] a
end type

dim mytype[] arr
dim mytype[] arr2
dim mytype t

block
	t.i = 101
	push(arr, t)
	print "here 1" arr[0].i
	
	t.i = 102
	arr[0] = t
	print "here 2" arr[0].i

	arr2 = arr
	print "here 3" arr2[0].i
end block