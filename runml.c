//  Platform:   Apple
// Complies with cc -std=c11 -Wall -Werror -o runml runml.c
// Invoke Trans-complier with ./runml test.ml [-v]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>   // For getpid()
#include <stdarg.h>   // For variable argument lists

#define MAX_LINE_LENGTH 256
#define MAX_IDENTIFIER_LENGTH 12
#define MAX_IDENTIFIERS 50
#define MAX_FUNCTIONS 50
#define MAX_GLOBAL_VARS 50

// Global verbose flag
int verbose = 0;

// Struct to hold function information
typedef struct {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    char parameters[MAX_IDENTIFIERS][MAX_IDENTIFIER_LENGTH + 1];
    char parameter_types[MAX_IDENTIFIERS][10];
    int parameter_count;
    char return_type[10];
    char body[MAX_LINE_LENGTH * MAX_IDENTIFIERS];
    int has_return_statement;
} Function;

// Struct to hold variable information
typedef struct {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    char type[10];
} Variable;

// Global array to store functions, global variables, and their types
Function functions[MAX_FUNCTIONS];
int function_count = 0;
Variable global_variables[MAX_GLOBAL_VARS];
int global_var_count = 0;
Variable local_variables[MAX_IDENTIFIERS];
int local_var_count = 0;

// Function declarations (forward declarations)
void usage(const char *program_name);
void debug_log(const char *log_type, const char *format, ...);
void error_log(const char *error_type, const char *format, ...);
FILE *open_ml_file(const char *ml_filename);
FILE *create_c_file();
void first_pass(FILE *ml_file);
void second_pass(FILE *ml_file, FILE *c_file);
int compile_c_program(pid_t pid);
int execute_c_program(pid_t pid, int argc, char *argv[]);
void clean_up(pid_t pid);
void store_function_definition_and_body(const char *line, FILE *file);
void store_variable(const char *line, int is_global);
void generate_global_variables(FILE *output_file);
void generate_function_prototypes_and_code(FILE *output_file);
void generate_main_code(FILE *ml_file, FILE *output_file);
void generate_c_code(const char *ml_code, FILE *output_file);
void parse_expression(const char *expr, FILE *output_file);
void parse_term_or_factor(const char *expr, FILE *output_file);
char *determine_variable_type(const char *value);
void generate_print_statement(FILE *output_file, const char *expression);
void determine_parameter_types(const char *function_call);
void update_function_prototype(const char *function_name);
int check_parentheses_balance(const char *line);
int is_valid_identifier(const char *name);
int check_type_consistency(const char *var_type, const char *value);
int check_function_variable_conflict(const char *var_name);

/**
 * Prints the usage information for the program.
 * @param program_name - Name of the program, typically argv[0].
 */
void usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <ml-file> [-v]\n", program_name);
}

/**
 * Debug log function for processing and code-related messages.
 * Automatically prepends either '@ Debug [INFO] : ' or '@ Debug [CODE] : ' based on the log_type.
 * 
 * @param log_type - Specifies the type of log, either "INFO" for processing-related logs or "CODE" for code-related logs.
 * @param format - The format string (similar to printf).
 * @param ... - The variable arguments to format and print.
 */
void debug_log(const char *log_type, const char *format, ...) {
    if (verbose) {
        va_list args;
        va_start(args, format);
        
        if (strcmp(log_type, "INFO") == 0) {
            printf("@ Debug [INFO] : ");
        } else if (strcmp(log_type, "CODE") == 0) {
            printf("@ Debug [CODE] : ");
        }
        
        vprintf(format, args);  // Print the formatted string
        va_end(args);
    }
}

/**
 * Error log function for displaying error messages.
 * Automatically prepends either '! Error [SYNTAX] : ' or '! Error [FILE] : ' based on the error_type.
 * 
 * @param error_type - Specifies the type of error, either "SYNTAX" or "FILE".
 * @param format - The format string (similar to printf).
 * @param ... - The variable arguments to format and print.
 * Exits the program immediately upon encountering a syntax error.
 */
void error_log(const char *error_type, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    if (strcmp(error_type, "SYNTAX") == 0) {
        fprintf(stderr, "! Error [SYNTAX] : ");
        vfprintf(stderr, format, args);  // Print the formatted error message to stderr
        exit(EXIT_FAILURE);
    } else if (strcmp(error_type, "FILE") == 0) {
        fprintf(stderr, "! Error [FILE] : ");
        vfprintf(stderr, format, args);  // Print the formatted error message to stderr
    }
    
    va_end(args);
}

/**
 * Opens the ml file for reading.
 * @param ml_filename - Path to the ml file.
 * @return - FILE pointer to the opened file.
 */
FILE *open_ml_file(const char *ml_filename) {
    FILE *ml_file = fopen(ml_filename, "r");
    if (!ml_file) {
        error_log("FILE", "Could not open file %s\n", ml_filename);
    } else {
        debug_log("INFO", "Opened file %s\n", ml_filename);
    }
    return ml_file;
}

/**
 * Creates a unique C file based on the process ID for storing the translated code.
 * @return - FILE pointer to the created C file.
 */
FILE *create_c_file() {
    pid_t pid = getpid();  // Get the process ID
    char c_filename[64];   // Buffer to hold the dynamically generated filename
    snprintf(c_filename, sizeof(c_filename), "ml_%d.c", pid);  // Format: ml_<PID>.c

    FILE *c_file = fopen(c_filename, "w");
    if (!c_file) {
        error_log("FILE", "Could not create temporary C file.\n");
    } else {
        debug_log("INFO", "Created temporary C file: %s\n", c_filename);
    }

    // Write necessary includes
    if (c_file) {
        fprintf(c_file, "#include <stdio.h>\n");
        fprintf(c_file, "#include <math.h>\n");  // Include math for fmod
    }

    return c_file;
}

/**
 * Checks if parentheses are balanced in the given line.
 * @param line - The string containing the code to check.
 * @return 1 if balanced, 0 if unbalanced.
 */
int check_parentheses_balance(const char *line) {
    int open_parens = 0;
    while (*line) {
        if (*line == '(') open_parens++;
        if (*line == ')') open_parens--;
        if (open_parens < 0) return 0; // Extra closing parenthesis
        line++;
    }
    return open_parens == 0;
}

/**
 * Checks if a given identifier is a valid variable name.
 * @param name - The variable name to check.
 * @return 1 if valid, 0 if invalid.
 */
int is_valid_identifier(const char *name) {
    if (!isalpha(name[0])) return 0;  // Must start with a letter
    for (int i = 1; name[i] != '\0'; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;  // Only alphanumeric or '_'
    }
    return strlen(name) <= MAX_IDENTIFIER_LENGTH;
}

/**
 * Determines the type of a variable based on its value in the expression.
 * @param value - The value assigned to the variable.
 * @return A string representing the type of the variable ("int" or "double").
 */
char *determine_variable_type(const char *value) {
    return (strchr(value, '.') != NULL) ? "double" : "int";
}

/**
 * Checks if a variable is being assigned a consistent type.
 * @param var_type - The expected type of the variable.
 * @param value - The value being assigned.
 * @return 1 if the types are consistent, 0 if not.
 */
int check_type_consistency(const char *var_type, const char *value) {
    const char *value_type = determine_variable_type(value);
    return strcmp(var_type, value_type) == 0;
}

/**
 * Checks if a given variable name conflicts with a function name.
 * @param var_name - The variable name to check.
 * @return 1 if there is no conflict, 0 if there is a conflict.
 */
int check_function_variable_conflict(const char *var_name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, var_name) == 0) {
            return 0;  // Conflict: variable name matches a function name
        }
    }
    return 1;
}

/**
 * First pass: Parse the ml file to store function definitions and global variables.
 * @param ml_file - FILE pointer to the ml file being parsed.
 */
void first_pass(FILE *ml_file) {
    char line[MAX_LINE_LENGTH];
    debug_log("INFO", "Starting first pass to parse global variables and functions\n");
    while (fgets(line, sizeof(line), ml_file)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline character

        if (!check_parentheses_balance(line)) {
            error_log("SYNTAX", "Unbalanced parentheses in line: %s\n", line);
            continue;
        }

        if (strncmp(line, "function", 8) == 0) {
            store_function_definition_and_body(line, ml_file);
        } else if (strstr(line, "<-")) {
            store_variable(line, 1);  // Store as a global variable
        }
    }
    rewind(ml_file);  // Rewind the file for the second pass
}

/**
 * Stores a function definition after validating its syntax.
 * Ensures all lines in the function body start with exactly one tab character.
 * 
 * @param line - A string representing the line containing the function definition.
 * @param file - The file pointer to continue reading the function body.
 */
void store_function_definition_and_body(const char *line, FILE *file) {
    if (function_count >= MAX_FUNCTIONS) {
        error_log("SYNTAX", "Too many functions defined.\n");
        return;
    }

    char function_name[MAX_IDENTIFIER_LENGTH + 1];
    char parameters[MAX_LINE_LENGTH];
    char param_buffer[MAX_LINE_LENGTH] = {0};  // To store parameters without parentheses
    int parameter_count = 0;
    int has_return_statement = 0;  // New variable to track return statement

    // Check for parentheses around parameters
    const char *pos = line + 9; // Skip "function"
    if (sscanf(pos, "%12s ( %[^)] )", function_name, parameters) == 2) {
        debug_log("CODE", "Function definition with parentheses: %s\n", function_name);
    } else if (sscanf(pos, "%12s %[^\n]", function_name, param_buffer) == 2) {
        debug_log("CODE", "Function definition without parentheses: %s\n", function_name);
        strncpy(parameters, param_buffer, sizeof(parameters) - 1);  // Store params
    } else {
        error_log("SYNTAX", "Invalid function definition: %s\n", line);
        return;
    }

    // Split the parameters by spaces or commas
    char *param = strtok(parameters, " ,");
    while (param != NULL) {
        if (strlen(param) > MAX_IDENTIFIER_LENGTH || !isalpha(param[0])) {
            error_log("SYNTAX", "Invalid parameter in function: %s\n", param);
            return;
        }
        // Store the parameter name
        strcpy(functions[function_count].parameters[parameter_count], param);
        strcpy(functions[function_count].parameter_types[parameter_count], "unknown");  // Initially unknown
        parameter_count++;

        param = strtok(NULL, " ,");
    }

    functions[function_count].parameter_count = parameter_count;
    strcpy(functions[function_count].name, function_name);
    strcpy(functions[function_count].return_type, "void");  // Initially assume void return type

    // Parse the function body
    char body_line[MAX_LINE_LENGTH];
    functions[function_count].body[0] = '\0';  // Initialize the body as an empty string

    while (fgets(body_line, sizeof(body_line), file)) {
        body_line[strcspn(body_line, "\n")] = '\0'; // Remove newline character

        // Detect the end of the function body: an empty line or non-indented line
        if (strlen(body_line) == 0 || body_line[0] != '\t') {
            // Peek the next two lines to check if we are outside the function
            char next_line_1[MAX_LINE_LENGTH];
            char next_line_2[MAX_LINE_LENGTH];

            long current_pos = ftell(file);  // Save current position

            fgets(next_line_1, sizeof(next_line_1), file);
            next_line_1[strcspn(next_line_1, "\n")] = '\0';

            fgets(next_line_2, sizeof(next_line_2), file);
            next_line_2[strcspn(next_line_2, "\n")] = '\0';

            fseek(file, current_pos, SEEK_SET);  // Restore position

            // If both next lines are not indented, we are outside the function body
            if (next_line_1[0] != '\t' && next_line_2[0] != '\t') {
                break;  // Exit the loop, no need to continue checking indentation
            } else if (next_line_1[0] == '\t') {
                // If the next line is indented correctly, report a syntax error
                error_log("SYNTAX", "Invalid indentation in function '%s'. Line has spaces or multiple tabs.\n", function_name);
            }

            continue;  // Move on to the next line
        }

        // Check for return statements and track their presence
        if (strncmp(body_line + 1, "return", 6) == 0) {
            has_return_statement = 1;  // Mark that a return statement is found
        }

        // Translate the line before adding it to the body
        FILE *temp_body = fopen("temp_body.c", "w+");
        generate_c_code(body_line + 1, temp_body);  // Skip the leading tab character
        fseek(temp_body, 0, SEEK_SET);  // Rewind the file to read its content

        char translated_line[MAX_LINE_LENGTH];
        while (fgets(translated_line, sizeof(translated_line), temp_body)) {
            strcat(functions[function_count].body, translated_line);
        }

        fclose(temp_body);
    }

    functions[function_count].has_return_statement = has_return_statement;  // Store return presence
    function_count++;
}

/**
 * Stores a variable and validates its name and type. 
 * Variables are stored either globally or locally.
 * @param line - The line containing the variable assignment.
 * @param is_global - Boolean to indicate if the variable is global (1) or local (0).
 */
void store_variable(const char *line, int is_global) {
    if ((is_global && global_var_count >= MAX_GLOBAL_VARS) || (!is_global && local_var_count >= MAX_IDENTIFIERS)) {
        error_log("SYNTAX", "Too many variables defined.\n");
        return;
    }

    char identifier[MAX_IDENTIFIER_LENGTH];
    char expression[MAX_LINE_LENGTH];
    char *type;

    if (sscanf(line, "%11s <- %[^\n]", identifier, expression) == 2) {
        // Check for valid variable name
        if (!is_valid_identifier(identifier)) {
            error_log("SYNTAX", "Invalid variable name: %s\n", identifier);
            return;
        }

        // Check for function-variable name conflict
        if (!check_function_variable_conflict(identifier)) {
            error_log("SYNTAX", "Variable name conflicts with a function name: %s\n", identifier);
            return;
        }

        type = determine_variable_type(expression);
        Variable *var_array = is_global ? global_variables : local_variables;
        int *var_count = is_global ? &global_var_count : &local_var_count;

        // Check type consistency
        if (!check_type_consistency(type, expression)) {
            error_log("SYNTAX", "Type mismatch for variable %s: expected %s but got %s\n", identifier, type, determine_variable_type(expression));
            return;
        }

        strcpy(var_array[*var_count].name, identifier);
        strcpy(var_array[*var_count].type, type);
        (*var_count)++;
    }
}

/**
 * Second pass: Generate the C code for the main function and other components.
 * @param ml_file - FILE pointer to the ml file being parsed.
 * @param c_file - FILE pointer to the C file where translated code will be written.
 */
void second_pass(FILE *ml_file, FILE *c_file) {
    debug_log("INFO", "Starting second pass to generate C code\n");

    // Generate global variables
    generate_global_variables(c_file);

    // Generate function prototypes and code
    generate_function_prototypes_and_code(c_file);

    // Start the main function
    fprintf(c_file, "int main(int argc, char *argv[]) {\n");

    // Generate the main function code
    generate_main_code(ml_file, c_file);

    // Close the main function in the C file
    fprintf(c_file, "return 0;\n}\n");
}

/**
 * Generates global variable declarations in the C output file.
 * @param output_file - The file pointer to write the generated C code.
 */
void generate_global_variables(FILE *output_file) {
    for (int i = 0; i < global_var_count; i++) {
        fprintf(output_file, "%s %s = 0.0;\n", global_variables[i].type, global_variables[i].name);
    }
    fprintf(output_file, "\n");
}

/**
 * Generates function prototypes and the corresponding function body for each defined function.
 * @param output_file - The file pointer to write the generated C code.
 */
void generate_function_prototypes_and_code(FILE *output_file) {
    for (int i = 0; i < function_count; i++) {
        Function *func = &functions[i];
        debug_log("CODE", "Generating prototype and code for function: %s\n", func->name);
        update_function_prototype(func->name);

        // Generate function prototype
        fprintf(output_file, "%s %s(", func->return_type, func->name);
        for (int j = 0; j < func->parameter_count; j++) {
            fprintf(output_file, "%s %s", func->parameter_types[j], func->parameters[j]);
            if (j < func->parameter_count - 1) {
                fprintf(output_file, ", ");
            }
        }
        fprintf(output_file, ");\n");

        // Generate function code
        fprintf(output_file, "%s %s(", func->return_type, func->name);
        for (int j = 0; j < func->parameter_count; j++) {
            fprintf(output_file, "%s %s", func->parameter_types[j], func->parameters[j]);
            if (j < func->parameter_count - 1) {
                fprintf(output_file, ", ");
            }
        }
        fprintf(output_file, ") {\n%s", func->body);

        // Add a default return statement only if there is no explicit return
        if (!func->has_return_statement && strcmp(func->return_type, "void") != 0) {
            fprintf(output_file, "return 0;\n");  // Add return 0 for non-void functions
        }

        fprintf(output_file, "}\n\n");
    }
}

/**
 * Generates the main code block by parsing each line of the ml file.
 * @param ml_file - File pointer to the input ml file.
 * @param output_file - File pointer to the generated C file.
 */
void generate_main_code(FILE *ml_file, FILE *output_file) {
    char line[MAX_LINE_LENGTH];
    local_var_count = 0;  // Reset local variables count for the main function
    while (fgets(line, sizeof(line), ml_file)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline character

        // Skip function definitions (they've already been processed)
        if (strncmp(line, "function", 8) == 0) {
            // Skip the function body lines
            while (fgets(line, sizeof(line), ml_file)) {
                if (line[0] != '\t') {
                    break;
                }
            }
            continue;
        }

        if (line[0] == '#') {
            debug_log("CODE", "Comment - %s\n", line);
            continue; // Skip comments
        }

        if (strstr(line, "<-")) {
            store_variable(line, 0);  // Store as a local variable
        }

        generate_c_code(line, output_file);
    }
}

/**
 * Translates a line of ml code into C code and writes it to the output file.
 * It handles assignment, print, return, and function call statements.
 * @param ml_code - The line of ml code to be translated.
 * @param output_file - The file pointer to write the translated C code.
 */
void generate_c_code(const char *ml_code, FILE *output_file) {
    if (strstr(ml_code, "<-")) {
        char identifier[MAX_IDENTIFIER_LENGTH];
        char expression[MAX_LINE_LENGTH];
        if (sscanf(ml_code, "%11s <- %[^\n]", identifier, expression) == 2) {
            debug_log("CODE", "Assignment - Identifier: %s, Expression: %s\n", identifier, expression);

            // Determine if the variable is global or local
            char *type = NULL;
            for (int i = 0; i < global_var_count; i++) {
                if (strcmp(global_variables[i].name, identifier) == 0) {
                    type = global_variables[i].type;
                    break;
                }
            }
            if (!type) {
                for (int i = 0; i < local_var_count; i++) {
                    if (strcmp(local_variables[i].name, identifier) == 0) {
                        type = local_variables[i].type;
                        break;
                    }
                }
            }
            if (!type) {
                type = determine_variable_type(expression);
                strcpy(local_variables[local_var_count].name, identifier);
                strcpy(local_variables[local_var_count].type, type);
                local_var_count++;
                fprintf(output_file, "%s %s = ", type, identifier);  // Local variable declaration
            } else {
                fprintf(output_file, "%s = ", identifier);  // Assignment to existing variable
            }

            parse_expression(expression, output_file);
            fprintf(output_file, ";\n");
            return;
        } else {
            error_log("SYNTAX", "Invalid assignment: %s\n", ml_code);
            return;
        }
    }

    if (strncmp(ml_code, "print ", 6) == 0) {
        char expression[MAX_LINE_LENGTH];
        sscanf(ml_code + 6, "%[^\n]", expression);  // Get everything after 'print '
        debug_log("CODE", "Print - Expression: %s\n", expression);
        generate_print_statement(output_file, expression);
        return;
    }

    if (strncmp(ml_code, "return ", 7) == 0) {
        char expression[MAX_LINE_LENGTH];
        sscanf(ml_code + 7, "%[^\n]", expression);  // Get everything after 'return '
        debug_log("CODE", "Return - Expression: %s\n", expression);
        // Translate return statement to C code
        fprintf(output_file, "return ");
        parse_expression(expression, output_file);
        fprintf(output_file, ";\n");
        return;
    }

    if (strchr(ml_code, '(') && strchr(ml_code, ')')) {
        debug_log("CODE", "Function Call - %s\n", ml_code);
        determine_parameter_types(ml_code);  // Determine parameter types for the function call
        fprintf(output_file, "%s;\n", ml_code); // Translate directly to C function call
        return;
    }

    error_log("SYNTAX", "Unrecognized statement: %s\n", ml_code);
}

/**
 * Parses an ml expression and generates its C equivalent.
 * Ensures that the expression is syntactically valid.
 * @param expr - The ml expression to parse.
 * @param output_file - The file pointer to write the parsed expression in C.
 */
void parse_expression(const char *expr, FILE *output_file) {
    int open_parens = 0;
    const char *term = expr;

    while (*expr != '\0') {
        if (*expr == '(') {
            open_parens++;
        } else if (*expr == ')') {
            open_parens--;
            if (open_parens < 0) {
                error_log("SYNTAX", "Unmatched closing parenthesis in expression: %s\n", term);
                return;
            }
        }

        // Check for invalid characters
        if (!isalnum(*expr) && !strchr("+-*/()., ", *expr)) {
            error_log("SYNTAX", "Invalid character in expression: %c\n", *expr);
            return;
        }

        expr++;
    }

    if (open_parens != 0) {
        error_log("SYNTAX", "Unmatched opening parenthesis in expression: %s\n", term);
        return;
    }

    // Proceed with parsing the expression...
    parse_term_or_factor(term, output_file);
}

/**
 * Parses a term or factor from an ml expression and generates its C equivalent.
 * This function handles multiplication, division, and parentheses.
 * @param expr - The ml expression term to parse.
 * @param output_file - The file pointer to write the parsed term in C.
 */
void parse_term_or_factor(const char *expr, FILE *output_file) {
    const char *term = expr;
    while (*expr != '\0' && *expr != '*' && *expr != '/') {
        expr++;
    }

    if (*expr == '*') {
        char left_term[MAX_LINE_LENGTH];
        strncpy(left_term, term, expr - term);
        left_term[expr - term] = '\0';
        debug_log("CODE", "Term - Left term: %s, Operator: *, Right term: %s\n", left_term, expr + 1);
        parse_term_or_factor(left_term, output_file);
        fprintf(output_file, " * ");
        parse_term_or_factor(expr + 1, output_file);
    } else if (*expr == '/') {
        char left_term[MAX_LINE_LENGTH];
        strncpy(left_term, term, expr - term);
        left_term[expr - term] = '\0';
        debug_log("CODE", "Term - Left term: %s, Operator: /, Right term: %s\n", left_term, expr + 1);
        parse_term_or_factor(left_term, output_file);
        fprintf(output_file, " / ");
        parse_term_or_factor(expr + 1, output_file);
    } else {
        debug_log("CODE", "Factor - %s\n", term);
        fprintf(output_file, "%s", term);
    }
}

/**
 * Determines the types of parameters in a function call by parsing the provided arguments.
 * Updates the function prototype to reflect the correct parameter types.
 * @param function_call - The string containing the function call with parameters.
 */
void determine_parameter_types(const char *function_call) {
    char function_name[MAX_IDENTIFIER_LENGTH + 1];
    char parameter_values[MAX_LINE_LENGTH];
    char *token;
    char *type;

    sscanf(function_call, "%12s(%[^\n])", function_name, parameter_values);

    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, function_name) == 0) {
            // Determine the parameter types from the values passed in the function call
            token = strtok(parameter_values, ", ");
            int j = 0;
            while (token != NULL && j < functions[i].parameter_count) {
                type = determine_variable_type(token);
                strcpy(functions[i].parameter_types[j], type);
                token = strtok(NULL, ", ");
                j++;
            }

            // Set the return type to the type of the first parameter (for simplicity)
            if (functions[i].parameter_count > 0) {
                strcpy(functions[i].return_type, functions[i].parameter_types[0]);
            }

            break;
        }
    }
}

/**
 * Updates the function prototype to ensure that parameter types are known and accurate.
 * If the parameter types are unknown, they are defaulted to "double".
 * @param function_name - The name of the function whose prototype needs updating.
 */
void update_function_prototype(const char *function_name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, function_name) == 0) {
            // Function prototype is updated when parameters are known
            if (functions[i].parameter_count > 0 && strcmp(functions[i].parameter_types[0], "unknown") == 0) {
                // If still unknown, set to default type (e.g., double)
                for (int j = 0; j < functions[i].parameter_count; j++) {
                    strcpy(functions[i].parameter_types[j], "double");
                }
                strcpy(functions[i].return_type, "double");
            }
            break;
        }
    }
}

/**
 * Generates a print statement in C from an ml print statement.
 * Ensures that integers and floating-point numbers are formatted correctly.
 * @param output_file - The file pointer to write the generated C code.
 * @param expression - The expression to be printed.
 */
void generate_print_statement(FILE *output_file, const char *expression) {
    fprintf(output_file, "{\n");
    fprintf(output_file, "double temp_value;\n");
    fprintf(output_file, "temp_value = ");
    parse_expression(expression, output_file);
    fprintf(output_file, ";\n");

    // Check if temp_value is an integer or not
    fprintf(output_file, "if (fabs(temp_value - (int)temp_value) < 1e-6) {\n");
    fprintf(output_file, "printf(\"%%d\\n\", (int)temp_value);\n");
    fprintf(output_file, "} else {\n");
    fprintf(output_file, "printf(\"%%.6f\\n\", temp_value);\n");
    fprintf(output_file, "}\n");

    fprintf(output_file, "}\n");
}

/**
 * Compiles the generated C file.
 * @param pid - The process ID, used for creating the unique filename.
 * @return - EXIT_SUCCESS on successful compilation, EXIT_FAILURE on error.
 */
int compile_c_program(pid_t pid) {
    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "cc -std=c11 -Wall -Werror -o ml_%d ml_%d.c", pid, pid);
    debug_log("INFO", "Compiling the C file with command: %s\n", compile_command);
    if (system(compile_command) != 0) {
        error_log("FILE", "Compilation failed for ml_%d.c\n", pid);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Executes the compiled C program.
 * @param pid - The process ID, used for creating the unique filename.
 * @param argc - The argument count.
 * @param argv - The argument vector (command-line arguments).
 * @return - EXIT_SUCCESS on successful execution, EXIT_FAILURE on error.
 */
int execute_c_program(pid_t pid, int argc, char *argv[]) {
    char execute_command[256];
    snprintf(execute_command, sizeof(execute_command), "./ml_%d", pid);

    // Append optional command-line arguments
    for (int i = 2; i < argc; i++) {
        strcat(execute_command, " ");
        strcat(execute_command, argv[i]);
    }

    debug_log("INFO", "Executing the compiled program with command: %s\n", execute_command);
    if (system(execute_command) != 0) {
        error_log("FILE", "Execution failed for ml_%d\n", pid);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Cleans up the temporary files (both the .c file and the compiled binary).
 * @param pid - The process ID, used for creating the unique filenames.
 */
void clean_up(pid_t pid) {
    debug_log("INFO", "Cleaning up temporary files\n");
    char c_filename[64];
    snprintf(c_filename, sizeof(c_filename), "ml_%d.c", pid);  // Format: ml_<PID>.c
    remove(c_filename);  // Remove the C file
    char binary_filename[64];
    snprintf(binary_filename, sizeof(binary_filename), "ml_%d", pid);
    remove(binary_filename);  // Remove the compiled program
}

/**
 * Main function of the runml transpiler.
 * Parses command-line arguments and controls the overall process.
 */
int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Check if verbose mode is enabled via the -v flag
    if (argc == 3 && strcmp(argv[2], "-v") == 0) {
        verbose = 1;
        debug_log("INFO", "Verbose mode enabled\n");
    }
    else if (argc == 3 && strcmp(argv[2], "-v") != 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *ml_filename = argv[1];

    // Open the ml file for reading
    FILE *ml_file = open_ml_file(ml_filename);
    if (!ml_file) {
        return EXIT_FAILURE;
    }

    // First pass: Parse and store function definitions and global variables
    first_pass(ml_file);

    // Create a temporary C file to store the translated code
    FILE *c_file = create_c_file();
    if (!c_file) {
        fclose(ml_file);
        return EXIT_FAILURE;
    }

    // Second pass: Generate the C code from the ml file
    second_pass(ml_file, c_file);

    fclose(ml_file);
    fclose(c_file);

    // Get the current process ID to create unique filenames
    pid_t pid = getpid();

    // Compile the C file
    if (compile_c_program(pid) != 0) {
        return EXIT_FAILURE;
    }

    // Execute the compiled C program
    if (execute_c_program(pid, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    // Clean up temporary files
    clean_up(pid);

    return EXIT_SUCCESS;
}