{
    "targets": [
        {
            "target_name": "LDAPCnx",
            "sources": [ "LDAP.cc", "LDAPCnx.cc", "LDAPCookie.cc", 
              "LDAPSASL.cc", "LDAPXSASL.cc", "SASLDefaults.cc" ],
            "include_dirs" : [
 	 	"<!(node -e \"require('nan')\")",
                "/usr/local/include"
	    ],
            "defines": [
                "LDAP_DEPRECATED"
            ],
            "ldflags": [
                "-L/usr/local/lib"
            ],
            "cflags": [
                "-Wall",
                "-g"
            ],
            "conditions": [
                [ "SASL==\"n\"", { "sources!": 
                  ["LDAPSASL.cc", "SASLDefaults.cc"] } ], 
                [ "SASL==\"y\"", { "sources!": ["LDAPXSASL.cc"] } ],
		['NODE_VERSION > 9', {
                  "conditions": [[
                    'OS=="linux"', {
                      "conditions": [[
                        "REDHAT_RELEASE == 6", {
                          "libraries": [ "../deps/RHEL6/libldap.a", "../deps/RHEL6/liblber.a" ]
                        }, {
                          "libraries": [ "../deps/RHEL7/libldap.a", "../deps/RHEL7/liblber.a" ]
                        }
                      ]]
                    }, {
                      "libraries": [ "../deps/OSX/libldap.a", "../deps/OSX/liblber.a" ],
                    }
                  ], [
		      "SASL==\"y\"", {
			  "libraries": [ "-lsasl2" ]
		      }
		  ]],
                  "include_dirs": [ "deps/include" ],
                  "libraries": [ "-lresolv" ]
		}, {
		    "libraries": [ "-lldap" ]
		}]
            ]
        }
    ],
    "variables": {
      "SASL": "<!(test -f /usr/include/sasl/sasl.h && echo y || echo n)",
      "NODE_VERSION": "<!(node --version | cut -d. -f1 | cut -dv -f2)",
      "REDHAT_RELEASE": "<!(test ! -e /etc/redhat-release || cat /etc/redhat-release | cut -d' ' -f3 | cut -d'.' -f 1)"
    },
    "conditions": [
        [
            "OS==\"mac\"",
            {
                "link_settings": {
                    "libraries": [
                        "-lldap"
                    ]
                },
                "xcode_settings": {
                    'OTHER_LDFLAGS': [
                        '-L/usr/local/lib'
                    ]
                }
            }
        ]
    ]
}

