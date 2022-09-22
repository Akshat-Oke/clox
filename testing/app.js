import hello from "./f.js";
console.log("in file app" + hello);
const a = "a";
//expected output should be:
//in file a
//in file b
//in file f
//in file app4
export default a;