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

(cog-set-value! wnull (*-open-*))
(cog-set-value! wnull (*-store-atom-*) (Concept "boffo"))
(cog-set-value! wnull (*-close-*))

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (cog-set-value! wnull (*-store-atom-*) (Concept "boffo")))
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

(cog-set-value! wmulti (*-open-*))
(cog-set-value! wmulti (*-store-atom-*) (Concept "boffo"))
(cog-set-value! wmulti (*-close-*))

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (cog-set-value! wmulti (*-store-atom-*) (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)


; ----------------------------
; Assorted permutations -- just one reader

(define rnull (ReadThruProxy "wnull"))
(cog-set-value! rnull parts (NullProxy "bar"))

(cog-set-value! rnull (*-open-*))
(cog-set-value! rnull (*-fetch-atom-*) (Concept "b1"))
(cog-set-value! rnull (*-fetch-atom-*) (Concept "b2"))
(cog-set-value! rnull (*-fetch-atom-*) (Concept "b3"))
(cog-set-value! rnull (*-fetch-atom-*) (Concept "b4"))
(cog-set-value! rnull (*-close-*))

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (cog-set-value! rnull (*-fetch-atom-*) (Concept "boffo")))
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

(cog-set-value! rmulti (*-open-*))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b1"))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b2"))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b3"))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b4"))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b5"))
(cog-set-value! rmulti (*-fetch-atom-*) (Concept "b6"))
(cog-set-value! rmulti (*-close-*))

(set! caught #f)
(catch 'C++-EXCEPTION (lambda () (cog-set-value! rmulti (*-fetch-atom-*) (Concept "boffo")))
	(lambda (key funcname msg)
		(format #t "Yes, we got the exception as expected\n")
		(set! caught #t)))

(test-assert "Failed to catch exception" caught)



(test-end tname)

(opencog-test-end)
