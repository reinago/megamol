# Copied from: https://github.com/travis-ci/travis-build/blob/master/lib/travis/build/bash/travis_retry.bash
# Copied on 29.07.2020, master is at c27f0b048a965a6824c3b31d731df7617b185b63.

travis_retry() {
  local result=0
  local count=1
  while [[ "${count}" -le 3 ]]; do
    [[ "${result}" -ne 0 ]] && {
      echo -e "\\n${ANSI_RED}The command \"${*}\" failed. Retrying, ${count} of 3.${ANSI_RESET}\\n" >&2
    }
    # run the command in a way that doesn't disable setting `errexit`
    "${@}"
    result="${?}"
    if [[ $result -eq 0 ]]; then break; fi
    count="$((count + 1))"
    sleep 1
  done

  [[ "${count}" -gt 3 ]] && {
    echo -e "\\n${ANSI_RED}The command \"${*}\" failed 3 times.${ANSI_RESET}\\n" >&2
  }

  return "${result}"
}
