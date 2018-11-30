//
//  main.swift
//  CoreOffset
//
//  Created by гык-sse2 on 29/11/2018.
//  Copyright © 2018 vit9696. All rights reserved.
//

import Foundation

var dict = [String : Any]()
var models = [String : String]()

let DocsDir = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)

let SMCDumpsDir = DocsDir.appendingPathComponent("SMCDumps", isDirectory: true)

for name in try! FileManager.default.contentsOfDirectory(atPath: SMCDumpsDir.path){
	let content = try! String.init(contentsOfFile: SMCDumpsDir.appendingPathComponent(name).path)
	let model = (name as NSString).deletingPathExtension
	
	if content.contains("TC0C") || content.contains("TC0c") {
		dict[model] = 0
	}
	else if content.contains("TC1C") || content.contains("TC1c") {
		dict[model] = 1
	}
	else {
		dict[model] = "No core temperature"
	}
}

let MacModels = DocsDir.appendingPathComponent("MacModels.txt")
let models_txt = try! String.init(contentsOfFile: MacModels.path)
for line in models_txt.split(separator: "\n") {
	let words = line.split(separator: " ")
	if words.count >= 2 {
		models[String(words[0])] = String(words[1])
	}
}


let SMCDatabaseDir = DocsDir.appendingPathComponent("SMCDatabase", isDirectory: true)

for boardID in try! FileManager.default.contentsOfDirectory(atPath: SMCDatabaseDir.path) {
	let content = try! String.init(contentsOfFile: SMCDatabaseDir.appendingPathComponent(boardID, isDirectory: true).appendingPathComponent("main.txt").path)
	let bid = (boardID as NSString).deletingPathExtension
	let model = models[bid] ?? bid
	if content.contains("TC0C") || content.contains("TC0c") {
		dict[model] = 0
	}
	else if content.contains("TC1C") || content.contains("TC1c") {
		dict[model] = 1
	}
	else {
		dict[model] = "No core temperature"
	}
}


let iStat = DocsDir.appendingPathComponent("iStat.txt")
let iStatContent = try! String.init(contentsOfFile: iStat.path)
var model = ""
var has0 = false
var has1 = false

for line in iStatContent.split(separator: "\n") {
	if line.starts(with: "Dumping ") {
		if model != "" {
			if has0 {
				dict[model] = 0
			}
			else if (has1) {
				dict[model] = 1
			}
			else {
				dict[model] = "No core temperature"
			}
		}
		model = String(line.split(separator: " ")[1])
		has0 = false
		has1 = false
	}
	else if line.contains("TC0C") || line.contains("TC0c") {
		has0 = true
	}
	else if line.contains("TC1C") || line.contains("TC1c") {
		has1 = true
	}
}


if model != "" {
	if has0 {
		dict[model] = 0
	}
	else if (has1) {
		dict[model] = 1
	}
	else {
		dict[model] = "No core temperature"
	}
}



let plist = try PropertyListSerialization.data(fromPropertyList: dict, format: PropertyListSerialization.PropertyListFormat.xml, options: 0)
print(String.init(data: plist, encoding: String.Encoding.utf8)!)
