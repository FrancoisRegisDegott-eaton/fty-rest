#!/bin/bash
# Note: bash-specific syntax is in fact used below, do not change the shebang!
#
# Copyright (C) 2015 - 2020 Eaton
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#! \file    testpass.sh
#  \brief   Helper script for REST API /admin/passwd
#  \author  Jim Klimov <EvgenyKlimov@Eaton.com>
#  \details Helper script for REST API /admin/passwd
#           reads the new username and password from stdin, validates it
#           against simple complexity checks (various character-type amounts)
#           and finally tests if cracklib thinks it is strong.
#           Return values: (11) = failed cracklib; (12) = failed complexity;
#           (13) = failed username presence; (14) = failed old passwd;
#           (0) = ok; (*) = misc errors
#           Verdicts on failures are prefixed with "BAD PASSWORD: " on stderr.

set -e

export PATH=/sbin:/usr/sbin:/bin:/usr/bin
export LC_ALL=C

# Some defaults from our default PAM config /etc/pam.d/fty-password
###    password   requisite       pam_cracklib.so enforce_for_root minlen=8 dcredit=-1 ocredit=-1 ucredit=0 lcredit=0
# Scoring follows cracklib: http://wpollock.com/AUnix2/PAM-Help.htm#cracklib
# + 1 point for each char of length
# + 1 for each named character type, up to a max of credit
# for negative numbers also require a minimum amount of those chars
SCORE_MIN=8
CHARS_TOTAL_MIN=4       # FIPS-112 minimum
CHARS_LOWER_MIN=0
CHARS_LOWER_CREDIT=0
CHARS_UPPER_MIN=0
CHARS_UPPER_CREDIT=0
CHARS_DIGIT_MIN=1
CHARS_DIGIT_CREDIT=1
CHARS_OTHER_MIN=1
CHARS_OTHER_CREDIT=1

PAMCFG=""
if [ -s /etc/pam.d/fty-password ] && [ -r /etc/pam.d/fty-password ]; then
    echo "Sourcing settings from /etc/pam.d/fty-password"
    PAMCFG="`egrep 'password.*req.*pam_cracklib\.so' /etc/pam.d/fty-password | egrep -v '^[ \t]*#' | (read _P _R _L OPTS; echo "$OPTS")`" || PAMCFG=""

    for T in $PAMCFG; do
        # Take the token after '=' (if any) and strip the first minus (if any)
        # NOTE: This is not necessarily a leading minus though
        V="`echo "$T" | (IFS== read K V; echo "$V" | (IFS=- read KK VV; echo "$KK$VV"))`" || V=""
        [[ -n "$V" ]] && [[ "$V" -ge 0 ]] && \
        case "$T" in
            minlen=*)	SCORE_MIN="$V" ;;       # Nothing for CHARS_TOTAL_MIN ?
            lcredit=-*)	CHARS_LOWER_MIN="$V" && CHARS_LOWER_CREDIT="$V" ;;
            lcredit=*)	CHARS_LOWER_MIN=0    && CHARS_LOWER_CREDIT="$V" ;;
            ucredit=-*)	CHARS_UPPER_MIN="$V" && CHARS_UPPER_CREDIT="$V" ;;
            ucredit=*)	CHARS_UPPER_MIN=0    && CHARS_UPPER_CREDIT="$V" ;;
            dcredit=-*)	CHARS_DIGIT_MIN="$V" && CHARS_DIGIT_CREDIT="$V" ;;
            dcredit=*)	CHARS_DIGIT_MIN=0    && CHARS_DIGIT_CREDIT="$V" ;;
            ocredit=-*)	CHARS_OTHER_MIN="$V" && CHARS_OTHER_CREDIT="$V" ;;
            ocredit=*)	CHARS_OTHER_MIN=0    && CHARS_OTHER_CREDIT="$V" ;;
        esac
    done
fi

if [[ -s /etc/default/bios-testpass ]] && [[ -r /etc/default/bios-testpass ]] ; then
    echo "Sourcing settings from /etc/default/bios-testpass"
    . /etc/default/bios-testpass
fi

die() {
    echo "${@}" >&2
    exit 1
}

usage() {
    echo "Usage: ( echo USER; echo NEWPASS; {echo OLDPASS;} ) | $0"
    echo "This script verifies sufficient complexity of the NEWPASS"
    echo "including comparisons against USER and OLDPASS (if available)"
}

diags_complexity() {
    echo "Current complexity-requirement settings:
        SCORE_MIN       $SCORE_MIN	CHARS_TOTAL_MIN  	$CHARS_TOTAL_MIN
        CHARS_LOWER_MIN $CHARS_LOWER_MIN	CHARS_LOWER_CREDIT	$CHARS_LOWER_CREDIT
        CHARS_UPPER_MIN $CHARS_UPPER_MIN	CHARS_UPPER_CREDIT	$CHARS_UPPER_CREDIT
        CHARS_DIGIT_MIN $CHARS_DIGIT_MIN	CHARS_DIGIT_CREDIT	$CHARS_DIGIT_CREDIT
        CHARS_OTHER_MIN $CHARS_OTHER_MIN	CHARS_OTHER_CREDIT	$CHARS_OTHER_CREDIT
"
}

check_passwd_cracklib() {
    local NEW_USER="$1"
    local NEW_PASSWD="$2"
    local RES=0

    [[ -n "${CRACKLIB_CHECK}" ]] && [[ -x "${CRACKLIB_CHECK}" ]] || \
        CRACKLIB_CHECK="`which cracklib-check 2>/dev/null | egrep '^/' | head -1`"
    [[ -n "${CRACKLIB_CHECK}" ]] && [[ -x "${CRACKLIB_CHECK}" ]] || \
        { echo "Missing cracklib-check program - test skipped" >&2
          return 0; }

    if [[ -n "${NEW_USER}" ]] && \
        /usr/bin/getent passwd ${NEW_USER} >/dev/null && \
        [[ "$(/usr/bin/id -u)" = 0 ]] \
    ; then      # Can 'su', so cracklib uses user info as well
        echo -e "Testing with cracklib-check program for user ${NEW_USER}... \c" >&2
        OUT="`su -c "${CRACKLIB_CHECK}" "${NEW_USER}" << EOF
${NEW_PASSWD}
EOF`" && \
        [[ -n "`egrep ': OK$' << EOF
${OUT}
EOF`" ]] || \
        RES=11
    else
        echo -e "Testing with cracklib-check program... \c" >&2
        OUT="`${CRACKLIB_CHECK} << EOF
${NEW_PASSWD}
EOF`" && \
        [[ -n "`egrep ': OK$' << EOF
${OUT}
EOF`" ]] || \
        RES=11
    fi

    if [[ "$RES" -gt 0 ]]; then
        echo "failed ($RES)" >&2
        echo "BAD PASSWORD: `cut -c $((${#NEW_PASSWD}+3))- << EOF
${OUT}
EOF`" >&2
        return $RES
    fi

    echo "succeeded" >&2
    return 0
}

check_passwd_complexity() {
    local NEW_USER="$1"
    local NEW_PASSWD="$2"

    local SCORE_TOTAL=0
    local SCORE_LOWER=0
    local SCORE_UPPER=0
    local SCORE_DIGIT=0
    local SCORE_OTHER=0
    local STRING=""
    local FAILED=""
    local COUNT_LOWER=0
    local COUNT_UPPER=0
    local COUNT_DIGIT=0
    local COUNT_OTHER=0

    # Note: intentionally no newline follows this print:
    printf 'Running a complexity check... ' >&2

    local COUNT_TOTAL="${#NEW_PASSWD}"
    if [[ "$COUNT_TOTAL" -lt "$CHARS_TOTAL_MIN" ]] ; then
        echo "failed" >&2
        echo "BAD PASSWORD: it is too short" >&2
        return 12
    fi

    STRING="`sed 's,[^[:lower:]],,g' << EOF
${NEW_PASSWD}
EOF`" && \
        COUNT_LOWER="${#STRING}"
    STRING="`sed 's,[^[:upper:]],,g' << EOF
${NEW_PASSWD}
EOF`" && \
        COUNT_UPPER="${#STRING}"
    STRING="`sed 's,[^[:digit:]],,g' << EOF
${NEW_PASSWD}
EOF`" && \
        COUNT_DIGIT="${#STRING}"
    STRING="`sed 's,[[:digit:][:lower:][:upper:]],,g' << EOF
${NEW_PASSWD}
EOF`" && \
        COUNT_OTHER="${#STRING}"

    if \
        [[ "$COUNT_LOWER" -lt "$CHARS_LOWER_MIN" ]] || \
        [[ "$COUNT_UPPER" -lt "$CHARS_UPPER_MIN" ]] || \
        [[ "$COUNT_DIGIT" -lt "$CHARS_DIGIT_MIN" ]] || \
        [[ "$COUNT_OTHER" -lt "$CHARS_OTHER_MIN" ]] \
    ; then
        echo "failed" >&2
        echo "BAD PASSWORD: not enough characters of various types" >&2
        return 12
    fi

    [[ "$COUNT_LOWER" -gt "$CHARS_LOWER_CREDIT" ]] && SCORE_LOWER="$CHARS_LOWER_CREDIT" || SCORE_LOWER="$COUNT_LOWER"
    [[ "$COUNT_UPPER" -gt "$CHARS_UPPER_CREDIT" ]] && SCORE_UPPER="$CHARS_UPPER_CREDIT" || SCORE_UPPER="$COUNT_UPPER"
    [[ "$COUNT_DIGIT" -gt "$CHARS_DIGIT_CREDIT" ]] && SCORE_DIGIT="$CHARS_DIGIT_CREDIT" || SCORE_DIGIT="$COUNT_DIGIT"
    [[ "$COUNT_OTHER" -gt "$CHARS_OTHER_CREDIT" ]] && SCORE_OTHER="$CHARS_OTHER_CREDIT" || SCORE_OTHER="$COUNT_OTHER"

    SCORE_TOTAL=$(($COUNT_TOTAL+$SCORE_LOWER+$SCORE_UPPER+$SCORE_DIGIT+$SCORE_OTHER))
    if [[ "$SCORE_TOTAL" -lt "$SCORE_MIN" ]] ; then
        echo "failed by overall complexity score" >&2
        echo "BAD PASSWORD: overall complexity score too low" >&2
        return 12
    fi

    echo "succeeded" >&2
    return 0
}

check_passwd_username() {
# Also request user name and check if its permutations
# are present in the password?
    local NEW_USER="$1"
    local NEW_PASSWD="$2"

    [[ -z "$NEW_USER" ]] && \
        echo "Missing username - test skipped" >&2 && \
        return 0

    echo -e "Running a username check against $NEW_USER... \c" >&2

    # Reduce chances for password leaking to envvars or command args history
    local LC_PASS="`tr '[:upper:]' '[:lower:]' << EOF
${NEW_PASSWD}
EOF`"
    local LC_USER="`echo "$NEW_USER" | tr '[:upper:]' '[:lower:]'`"

    if  [[ "$LC_USER" =~ "$LC_PASS" ]] || \
        [[ "$LC_PASS" =~ "$LC_USER" ]] \
    ; then
        echo "failed" >&2
        echo "BAD PASSWORD: too similar to the username" >&2
        return 13
    fi

    echo "succeeded" >&2
    return 0
}

check_passwd_oldpasswd() {
    local NEW_USER="$1"
    local NEW_PASSWD="$2"
    local OLD_PASSWD="$3"

    [[ -z "${OLD_PASSWD}" ]] && \
        echo "Missing old password - test skipped" >&2 && \
        return 0

    echo -e "Running a check against old password... \c" >&2
    if [[ "${OLD_PASSWD}" = "${NEW_PASSWD}" ]] ; then
        echo "failed" >&2
        echo "BAD PASSWORD: too similar to the old password" >&2
        return 14
    fi

    local LC_NPASS="`tr '[:upper:]' '[:lower:]' << EOF
${NEW_PASSWD}
EOF`"
    local LC_OPASS="`tr '[:upper:]' '[:lower:]' << EOF
${OLD_PASSWD}
EOF`"

    if  [[ "$LC_NPASS" =~ "$LC_OPASS" ]] || \
        [[ "$LC_OPASS" =~ "$LC_NPASS" ]] \
    ; then
        echo "failed" >&2
        echo "BAD PASSWORD: too similar to the old password" >&2
        return 14
    fi

    echo "succeeded" >&2
    return 0
}

check_passwd() {
    local NEW_USER="$1"
    local NEW_PASSWD="$2"

    check_passwd_cracklib "$@" && \
    check_passwd_complexity "$@" && \
    check_passwd_username "$@" && \
    { if [[ -n "$NEW_USER" ]] && [[ "x${NEW_USER}" != "x${USER}" ]] ; then
        check_passwd_username "${USER}" "${NEW_PASSWD}"
      else true; fi; } && \
    check_passwd_oldpasswd "$@"
}

read NEW_USER || { usage; die "Bad input"; }
read NEW_PASSWD || { usage; die "Bad input"; }
read OLD_PASSWD || OLD_PASSWD=""        # Optional

[[ -n "${NEW_PASSWD}" ]] || { usage; die "Bad input: new password is empty"; }

#[[ -n "${NEW_USER}" ]] || die "new username is empty"
#if ! /usr/bin/getent passwd ${NEW_USER} >/dev/null; then
#    die "User ${NEW_USER} not found by /usr/bin/getent passwd"
#fi

while [ $# -gt 0 ]; do
    case "$1" in
        -d) diags_complexity ;;
        -h) usage; exit 0;;
    esac
    shift
done

check_passwd "${NEW_USER}" "${NEW_PASSWD}" "${OLD_PASSWD}"
