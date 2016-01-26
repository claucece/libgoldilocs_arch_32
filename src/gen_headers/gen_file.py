from curve_data import curve_data, field_data
from textwrap import dedent

def redoc(filename,doc,author):
    doc = doc.replace("\n","\n * ")
    doc = dedent("""
        /**
         * @file %(filename)s
         * @author %(author)s
         *
         * @copyright
         *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \\n
         *   Released under the MIT License.  See LICENSE.txt for license information.
         *
         * %(doc)s
         *
         * @warning This file was automatically generated in Python.
         * Please do not edit it.
         */""") % { "filename": filename, "doc": doc, "author" : author }
    doc = doc.replace(" * \n", " *\n")
    return doc[1:]

gend_files = {}

per_map = {"field":field_data, "curve":curve_data, "global":{"global":{}} }

def gen_file(public,name,doc,code,per="global",author="Mike Hamburg"):
    is_header = name.endswith(".h") or name.endswith(".hxx") or name.endswith(".h++")
    
    for curve,data in per_map[per].iteritems():
        ns_name = name % data
        
        _,_,name_base = ns_name.rpartition("/")
        header_guard = "__" + ns_name.replace(".","_").replace("/","_").upper() + "__"
        
        ns_doc = dedent(doc).strip().rstrip()
        ns_doc = redoc(ns_name, ns_doc % data, author)
        ns_code = code % data
        ret = ns_doc + "\n"
        
        if is_header:
            ns_code = dedent("""\n
                #ifndef %(header_guard)s
                #define %(header_guard)s 1
                %(code)s
                #endif /* %(header_guard)s */
                """) % { "header_guard" : header_guard, "code": ns_code }
        ret += ns_code[1:-1]
        
        gend_files[ns_name] = (public,ret)