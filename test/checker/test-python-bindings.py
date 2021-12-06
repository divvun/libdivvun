#!/usr/bin/python3

import libdivvun

spec = libdivvun.ArCheckerSpec("sme.zcheck")
smegram = spec.getChecker("smegram", True)


def test(got, want):
    if got != want:
        print("Wanted '{}' but got '{}'".format(want, got))
        assert(got == want)


inp = "ja seammas ballat ođđa dieđuiguin"
for _ in range(1, 10):
    errs = libdivvun.proc_errs_bytes(smegram, inp)
    test(errs[0].form, 'dieđuiguin')
    test(errs[0].beg, 23)
    test(errs[0].end, 33)
    test(errs[0].msg, "msyn thingy")
    test(errs[0].err, "msyn-valency-loc-com")
    test(errs[0].rep, ('diehtukorrekt',))

inp2 = "Čoahkkinjoiheaddji gohčču"
errs2 = libdivvun.proc_errs_bytes(smegram, inp2)
test(errs2[0].rep, ('Čoahkkinjođiheaddji',))
