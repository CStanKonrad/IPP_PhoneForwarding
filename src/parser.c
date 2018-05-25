//
// Created by skonrad on 25/05/18.
//

#include <assert.h>
#include <stdint.h>
#include "character.h"
#include "input.h"
#include "parser.h"

struct Parser parserCreateNew() {
    struct Parser result;
    result.readedBytes = 0;
    result.isError = false;
    return result;
}


static int parserCharacterCanBeSkipped(int characterCode) {
    return characterIsWhite(characterCode)
            || characterIsNewLine(characterCode);
}

static bool parserSkipCharactersThatCanBeSkipped(Parser parser) {
    if (parserFinished(parser)) {
        return false;
    }

    size_t skipped = inputIgnoreWhile(parserCharacterCanBeSkipped);
    parser->readedBytes += skipped;
    return skipped != 0;
}

static bool parserSkipComments(Parser parser) {
    if (parserFinished(parser)
            || inputPeekCharacter() != PARSER_COMMENT_SEQUENCE[0]) {
        return false;
    }

    inputGetCharacter();
    parser->readedBytes++;

    if (inputPeekCharacter() != PARSER_COMMENT_SEQUENCE[1]) {
        parser->isError = true;
        return false;
    }

    inputGetCharacter();
    parser->readedBytes++;


    while (true) {
        int r = inputGetCharacter();
        parser->readedBytes++;
        if (parserFinished(parser)) {
            if (characterIsEOF(r)) {
                parser->readedBytes--;
            }
            parser->isError = true;
            return false;
        } else if (r == PARSER_COMMENT_SEQUENCE[0]
                && inputPeekCharacter() == PARSER_COMMENT_SEQUENCE[1]) {
            inputGetCharacter();
            parser->readedBytes++;

            return true;
        }
    }
}

void parserSkipSkipable(Parser parser) {
    while (parserSkipCharactersThatCanBeSkipped(parser)
           || parserSkipComments(parser));
}

bool parserError(Parser parser) {
    return parser->isError;
}

bool parserFinished(Parser parser) {
    return parserError(parser) || inputIsEOF();
}

static int parserIsSingleCharacterOperator(int characterCode) {
    return characterCode == PARSER_OPERATOR_QM
           || characterCode == PARSER_OPERATOR_REDIRECT;
}

int parserNextType(Parser parser) {
    if (parserFinished(parser)) {
        return PARSER_FAIL;
    }

    int c = inputPeekCharacter();

    if (characterIsDigit(c)) {
        return PARSER_ELEMENT_TYPE_NUMBER;
    } else if (characterIsLetter(c)) {
        return PARSER_ELEMENT_TYPE_WORD;
    } else if (parserIsSingleCharacterOperator(c)) {
        return PARSER_ELEMENT_TYPE_SINGLE_CHARACTER_OPERATOR;
    } else {
        inputGetCharacter();
        parser->readedBytes++;
        parser->isError = true;

        return PARSER_FAIL;
    }
}

int parserReadOperator(Parser parser) {
    if (parserFinished(parser)) {
        return PARSER_FAIL;
    }

    int ch = inputGetCharacter();
    parser->readedBytes++;
    if (parserIsSingleCharacterOperator(ch)) {
        if (ch == PARSER_OPERATOR_QM) {
            return PARSER_ELEMENT_TYPE_OPERATOR_QM;
        } else if (ch == PARSER_OPERATOR_REDIRECT) {
            return PARSER_ELEMENT_TYPE_OPERATOR_REDIRECT;
        } else {
            parser->isError = true;
            return PARSER_FAIL;
        }
    } else {
        const char *toCmp;
        int result;
        if (ch == PARSER_OPERATOR_NEW[0]) {
            toCmp = PARSER_OPERATOR_NEW + 1;
            result = PARSER_ELEMENT_TYPE_OPERATOR_NEW;
        } else if (ch == PARSER_OPERATOR_DELETE[0]) {
            toCmp = PARSER_OPERATOR_DELETE + 1;
            result = PARSER_ELEMENT_TYPE_OPERATOR_DELETE;
        } else {
            parser->isError = true;
            return PARSER_FAIL;
        }

        const char *ptr;
        for (ptr = toCmp; *ptr != '\0'; ptr++) {
            ch = inputGetCharacter();
            parser->readedBytes++;
            if (ch != *ptr) {
                parser->isError = true;
                return PARSER_FAIL;
            }
        }

        if (!parserCharacterCanBeSkipped(inputPeekCharacter())
                && !inputPeekCharacter() == PARSER_COMMENT_SEQUENCE[0]) {
            parser->isError = true;
            return PARSER_FAIL;
        }

        return result;

    }
}

static int parserIsLetterOrDigit(int characterCode) {
    return characterIsLetter(characterCode) || characterIsDigit(characterCode);
}

bool parserReadIdentificator(Parser parser, Vector destination) {
    size_t prefSize = vectorSize(destination);
    int readStatus = inputReadWhile(parserIsLetterOrDigit, SIZE_MAX, destination);
    parser->readedBytes += vectorSize(destination) - prefSize;


    parser->readedBytes++;
    if (!parserCharacterCanBeSkipped(inputGetCharacter())
            && !parserFinished(parser)) {
        parser->isError = true;
    }

    return readStatus == INPUT_READ_SUCCESS;
}

bool parserReadNumber(Parser parser, Vector destination) {
    size_t prefSize = vectorSize(destination);
    int readStatus = inputReadWhile(characterIsDigit, SIZE_MAX, destination);
    parser->readedBytes += vectorSize(destination) - prefSize;

    char tmp = inputPeekCharacter();
    if (!parserCharacterCanBeSkipped(inputPeekCharacter())
        && !parserFinished(parser)
            && !parserIsSingleCharacterOperator(inputPeekCharacter())) {
        parser->isError = true;
    }

    return readStatus == INPUT_READ_SUCCESS;
}



