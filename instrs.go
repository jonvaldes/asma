package main

import (
	"io/ioutil"
	"strings"
	 "fmt"
	"os"
	"os/exec"
)

func main(){
	data, _ := ioutil.ReadFile("instructions.txt")
	lines := strings.Split(string(data), "\n")

	outputFile, err := os.Create("generated/encoding.hxx")
	if err != nil{
		fmt.Println(err.Error())
		return
	}
	defer outputFile.Close()

	fmt.Fprintln(outputFile, "/*")

	var instrName string
	var literalBytes int
	var fileLines []string

	var encodings []string
	var instructionNames []string
	maxInstructionSize := 0

	for _, l := range lines {
		if len(l) == 0 {
			break
		}
		if l[0] == '/' {
			if len(fileLines) > 0 {
				fmt.Fprintln(outputFile, "\n", instrName)
				fileName := "generated/__" + instrName + ".s"
				err := ioutil.WriteFile(fileName, []byte("[bits 64]\n" + strings.Join(fileLines,"\n")), 0666)
				if err != nil{
					fmt.Println(err.Error())
					return
				}
				objFileName := "generated/__"+instrName+".obj"
				c := exec.Command("nasm", "-f", "win64", "-o",objFileName, fileName)
				c.Stdout = os.Stdout
				c.Stderr = os.Stderr

				if err = c.Run(); err != nil {
					fmt.Println(err.Error())
					return
				}

				data, err := exec.Command("dumpbin.exe", "/disasm", "/nologo", objFileName).Output()
				if err !=nil {
					fmt.Println(err.Error())
					return
				}
				dataLines := strings.Split(string(data), "\n")
				var bytesList []string

				for i:= 5; i < len(dataLines) - 5; i++ {
					s := strings.Replace(dataLines[i], "\r", "", -1)
					fmt.Fprintln(outputFile, s[19:])

					last := 39
					if last > len(s){
						last = len(s)
					}
					for _, b := range strings.Split(s[19:last], " ") {
						if len(b) >0 {
							bytesList = append(bytesList, "0x" + b)
						}
					}
				}
				if len(bytesList) > maxInstructionSize {
					maxInstructionSize = len(bytesList)
				}

				encodings = append(encodings, fmt.Sprintf(`{Instr_%s, "%s", %d, %d, {%s}},`, instrName, instrName, len(bytesList), literalBytes, strings.Join(bytesList, ", ")))
				instructionNames = append(instructionNames, instrName)

				// Next file

				fileLines = []string{}
			}

			instrName = strings.Split(l, " ")[1]

			literalBytes = 0
			if strings.Contains(l, "0xBAADBEEFBAADBEEF"){
				literalBytes = 8
			}else if strings.Contains(l, "0xBAADBEEF"){
				literalBytes = 4
			}
		}else{
			fileLines = append(fileLines, l)
		}
	}

	fmt.Fprintln(outputFile, "\n*/\n")

	fmt.Fprintln(outputFile, "enum Instr {")
	for _, i := range instructionNames {
		fmt.Fprintln(outputFile, "\tInstr_" + i + ",")
	}
	fmt.Fprintln(outputFile, "\n	Instr_CNT")
	fmt.Fprintln(outputFile, "};\n")

	fmt.Fprintf(outputFile, `struct Instruction{
	Instr		 instr;
	const char * name;
	i32			 size;
	i32			 literalBytes;
	u8  		 data[%d];
};
`, maxInstructionSize)

	fmt.Fprintln(outputFile, "\nconst Instruction g_instructions[Instr_CNT] = {\n")


	for _, e := range encodings{
		fmt.Fprintln(outputFile, "	Instruction "+e)
	}
	fmt.Fprintln(outputFile, "};")

}
