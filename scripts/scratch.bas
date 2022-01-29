module test

type mytype
	dim i
	dim int[] a
end type

dim mytype[] arr
dim mytype[] arr2
dim mytype t
dim mytype t2

block
	t.i = 101
	push(arr, t)
	print "here 1" arr[0].i
	
	t.i = 102
	arr[0] = t
	print "here 2" arr[0].i

	arr2 = arr
	print "here 3" arr2[0].i

	push(t.a, 123)
	print "fart"  t.a[0]
	print "fart"

	t2 = t
	t.a[0] = 400
	print "fart 2"  t.a[0]  t2.a[0]

	default(t)
	print t.i len(t.a)
end block