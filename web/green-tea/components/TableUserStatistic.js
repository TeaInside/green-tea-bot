import UserStatisticRow from "./UserStatisticRow"

export default function TableUserStatistic() {
    return (
        <div className="bg-green-500 overflow-y-scroll row-span-1">
            <h1 className="text-center bg-green-800 text-[30px]">User Stats</h1>
            <h1 className="text-center">Koding Teh</h1>
            <table className="w-10/12 mx-auto text-center border-solid border-4">
                <thead className="border-solid border-2">
                    <tr>
                        <th className="w-0 border-solid border-2">No.</th>
                        <th className="w-24 border-solid border-2">Photo</th>
                        <th className="w-32 border-solid border-2">Name</th>
                        <th className="w-24 border-solid border-2">Messages</th>
                    </tr>
                </thead>
                <tbody className="border-solid border-1">
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                    <UserStatisticRow />
                </tbody>
            </table>
        </div>
    )
}