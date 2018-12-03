//
//  main.swift
//  CoreOffset
//
//  Created by гык-sse2 on 29/11/2018.
//  Copyright © 2018 vit9696. All rights reserved.
//

import Foundation

var dict = [String : AnyHashable]()
var models = [String : String]()

func addValue(_ model : String, _ newValue : AnyHashable, _ source : String) {
	if let oldValue = dict[model] {
		if oldValue != newValue {
			if oldValue == ("No core temperature" as AnyHashable) {
				// We don't want "No core temperature"
				dict[model] = newValue
			}
			print("Value mismatch for \(model): was \(oldValue), new \(newValue), new source: \(source)")
		}
	}
	else {
		dict[model] = newValue
	}
}

let DocsDir = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)

let SMCDumpsDir = DocsDir.appendingPathComponent("SMCDumps", isDirectory: true)

for name in try! FileManager.default.contentsOfDirectory(atPath: SMCDumpsDir.path){
	let content = try! String.init(contentsOfFile: SMCDumpsDir.appendingPathComponent(name).path)
	let model = (name as NSString).deletingPathExtension
	
	if content.contains("TC0C") || content.contains("TC0c") {
		addValue(model, 0, "dump")
	}
	else if content.contains("TC1C") || content.contains("TC1c") {
		addValue(model, 1, "dump")
	}
	else {
		addValue(model, "No core temperature", "dump")
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
		addValue(model, 0, "firmware")
	}
	else if content.contains("TC1C") || content.contains("TC1c") {
		addValue(model, 1, "firmware")
	}
	else {
		addValue(model, "No core temperature", "firmware")
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
				addValue(model, 0, "iStat")
			}
			else if has1 {
				addValue(model, 1, "iStat")
			}
			else {
				addValue(model, "No core temperature", "iStat")
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
		addValue(model, 0, "iStat")
	}
	else if has1 {
		addValue(model, 1, "iStat")
	}
	else {
		addValue(model, "No core temperature", "iStat")
	}
}



let plist = try PropertyListSerialization.data(fromPropertyList: dict, format: PropertyListSerialization.PropertyListFormat.xml, options: 0)
print(String.init(data: plist, encoding: String.Encoding.utf8)!)

print("static const char *one_indexed_models[] = {")
print(dict.filter({$0.value as? Int == 1}).map({"\t\"\($0.key)\""}).sorted {$0.localizedStandardCompare($1) == .orderedAscending}.joined(separator: ",\n"))
print("};")
