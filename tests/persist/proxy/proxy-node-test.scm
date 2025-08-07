;
; proxy-node-test.scm -- Unit test for basic ProxyNode syntax.
;
(use-modules (opencog))
(use-modules (opencog exec))
(use-modules (opencog test-runner))

; Unit test fails if run by hand, and this module is not
; installed. So avoid that annoyance, and twinkle the module
; load path.
(if (resolve-module (list 'opencog 'persist) #:ensure #f)
	#t
	(add-to-load-path "../../../build/opencog/scm"))
(format #t "Load path is: ~A\n" %load-path)

(use-modules (opencog persist))

; ---------------------------------------------------------------------
(opencog-test-runner)
(define tname "proxy-node")
(test-begin tname)

(define parts (Predicate "*-proxy-parts-*"))

; This should lead to failure.
(define foo (WriteThruProxy "foo"))

(define caught #f)
(catch 'C++-EXCEPTION
	(lambda ()
		(cog-set-value! foo parts (Concept "bar")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)

; ----------------------------
; Assorted permutations -- just one target

(define wnull (WriteThruProxy "wnull"))
(cog-set-value! wnull parts (NullProxy "bar"))

(cog-open wnull)
(store-atom (Concept "boffo"))
(cog-close wnull)

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (store-atom (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)

; ----------------------------
; Multiple targets

(define wmulti (WriteThruProxy "wmulti"))
(cog-set-value! wmulti parts
	(List
		(NullProxy "foo")
		(NullProxy "bar")
		(NullProxy "baz")))

(cog-open wmulti)
(store-atom (Concept "boffo"))
(cog-close wmulti)

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (store-atom (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)


; ----------------------------
; Assorted permutations -- just one reader

(define rnull (ReadThruProxy "wnull"))
(cog-set-value! rnull parts (NullProxy "bar"))

(cog-open rnull)
(fetch-atom (Concept "b1"))
(fetch-atom (Concept "b2"))
(fetch-atom (Concept "b3"))
(fetch-atom (Concept "b4"))
(cog-close rnull)

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (fetch-atom (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)

; ----------------------------
; Multiple readers

(define rmulti (ReadThruProxy "wmulti"))
(cog-set-value! rmulti parts
	(List
		(NullProxy "foo")
		(NullProxy "bar")
		(NullProxy "baz")))

(cog-open rmulti)
(fetch-atom (Concept "b1"))
(fetch-atom (Concept "b2"))
(fetch-atom (Concept "b3"))
(fetch-atom (Concept "b4"))
(fetch-atom (Concept "b5"))
(fetch-atom (Concept "b6"))
(cog-close rmulti)

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (fetch-atom (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)



(test-end tname)

(opencog-test-end)
